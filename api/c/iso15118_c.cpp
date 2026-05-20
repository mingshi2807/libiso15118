// SPDX-License-Identifier: Apache-2.0
// 2024 Vedecom Contributors to EVerest
#include "iso15118_c.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <iso15118/d20/config.hpp>
#include <iso15118/d20/control_event.hpp>
#include <iso15118/d20/session.hpp>
#include <iso15118/d20/state/supported_app_protocol.hpp>
#include <iso15118/detail/helper.hpp>
#include <iso15118/io/connection_plain.hpp>
#include <iso15118/io/poll_manager.hpp>
#include <iso15118/io/sdp_server.hpp>
#include <iso15118/message/common_types.hpp>
#include <iso15118/session/feedback.hpp>
#include <iso15118/session/iso.hpp>

namespace io = iso15118::io;
namespace d20 = iso15118::d20;
namespace dt = iso15118::message_20::datatypes;
namespace m20 = iso15118::message_20;

// ── Minimal JSON helpers ──────────────────────────────────

// Crude JSON value type — just enough for our protocol.
struct JsonVal {
    enum Kind {
        Null,
        BoolVal,
        Number,
        String,
        Object,
        Array
    };
    Kind kind{Null};
    bool b{false};
    double num{0};
    std::string str;
    std::vector<std::pair<std::string, JsonVal>> fields;
    std::vector<JsonVal> items;

    const JsonVal* find(const char* key) const {
        for (auto& f : fields)
            if (f.first == key)
                return &f.second;
        return nullptr;
    }
    bool is(const char* k) const {
        return find(k) != nullptr;
    }

    // Convenience accessors (return defaults if missing).
    bool get_bool(const char* k, bool def = false) const {
        auto* v = find(k);
        return (v && v->kind == BoolVal) ? v->b : def;
    }
    double get_num(const char* k, double def = 0) const {
        auto* v = find(k);
        return (v && v->kind == Number) ? v->num : def;
    }
    std::string get_str(const char* k) const {
        auto* v = find(k);
        return (v && v->kind == String) ? v->str : std::string{};
    }
};

// Minimal recursive-descent JSON parser (no external deps).
class MiniJson {
public:
    static std::optional<JsonVal> parse(const char* s) {
        ctx_ = s;
        skip_ws();
        auto v = parse_value();
        if (!v)
            return std::nullopt;
        return v;
    }

private:
    static const char* ctx_;

    static char peek() {
        return *ctx_;
    }
    static char next() {
        return *ctx_++;
    }
    static void skip_ws() {
        while (*ctx_ == ' ' || *ctx_ == '\t' || *ctx_ == '\n' || *ctx_ == '\r')
            ++ctx_;
    }

    static std::optional<JsonVal> parse_value() {
        skip_ws();
        switch (peek()) {
        case '{':
            return parse_object();
        case '[':
            return parse_array();
        case '"':
            return parse_string();
        case 't':
        case 'f':
            return parse_bool();
        case 'n':
            return parse_null();
        default:
            return parse_number();
        }
    }

    static std::optional<JsonVal> parse_object() {
        next(); // '{'
        JsonVal v;
        v.kind = JsonVal::Object;
        skip_ws();
        if (peek() == '}') {
            next();
            return v;
        }
        for (;;) {
            skip_ws();
            if (peek() != '"')
                return std::nullopt;
            auto key = parse_string();
            if (!key)
                return std::nullopt;
            skip_ws();
            if (next() != ':')
                return std::nullopt;
            auto val = parse_value();
            if (!val)
                return std::nullopt;
            v.fields.emplace_back(key->str, std::move(*val));
            skip_ws();
            if (peek() == '}') {
                next();
                return v;
            }
            if (next() != ',')
                return std::nullopt;
        }
    }

    static std::optional<JsonVal> parse_array() {
        next(); // '['
        JsonVal v;
        v.kind = JsonVal::Array;
        skip_ws();
        if (peek() == ']') {
            next();
            return v;
        }
        for (;;) {
            auto item = parse_value();
            if (!item)
                return std::nullopt;
            v.items.push_back(std::move(*item));
            skip_ws();
            if (peek() == ']') {
                next();
                return v;
            }
            if (next() != ',')
                return std::nullopt;
        }
    }

    static std::optional<JsonVal> parse_string() {
        next(); // '"'
        JsonVal v;
        v.kind = JsonVal::String;
        while (peek() != '"') {
            if (peek() == '\\') {
                next();
            }
            if (peek() == 0)
                return std::nullopt;
            v.str += next();
        }
        next(); // '"'
        return v;
    }

    static std::optional<JsonVal> parse_number() {
        JsonVal v;
        v.kind = JsonVal::Number;
        char* end = nullptr;
        v.num = std::strtod(ctx_, &end);
        if (end == ctx_)
            return std::nullopt;
        ctx_ = end;
        return v;
    }

    static std::optional<JsonVal> parse_bool() {
        JsonVal v;
        v.kind = JsonVal::BoolVal;
        if (std::strncmp(ctx_, "true", 4) == 0) {
            v.b = true;
            ctx_ += 4;
            return v;
        }
        if (std::strncmp(ctx_, "false", 5) == 0) {
            v.b = false;
            ctx_ += 5;
            return v;
        }
        return std::nullopt;
    }

    static std::optional<JsonVal> parse_null() {
        if (std::strncmp(ctx_, "null", 4) != 0)
            return std::nullopt;
        ctx_ += 4;
        return JsonVal{};
    }
};
const char* MiniJson::ctx_ = nullptr;

// ── Config parsing ────────────────────────────────────────

d20::DcTransferLimits parse_dc_limits(const JsonVal& v) {
    d20::DcTransferLimits lim;
    auto* c = v.find("charge_limits");
    if (c) {
        auto* p = c->find("power");
        if (p) {
            lim.charge_limits.power.max = {static_cast<int16_t>(p->get_num("max")), 0};
            lim.charge_limits.power.min = {static_cast<int16_t>(p->get_num("min")), 0};
        }
        auto* cur = c->find("current");
        if (cur) {
            lim.charge_limits.current.max = {static_cast<int16_t>(cur->get_num("max")), 0};
        }
    }
    auto* volt = v.find("voltage");
    if (volt) {
        lim.voltage.max = {static_cast<int16_t>(volt->get_num("max")), 0};
    }
    auto* disc = v.find("discharge_limits");
    if (disc) {
        auto& dl = lim.discharge_limits.emplace();
        auto* dp = disc->find("power");
        if (dp) {
            dl.power.max = {static_cast<int16_t>(dp->get_num("max")), 0};
            dl.power.min = {static_cast<int16_t>(dp->get_num("min")), 0};
        }
        auto* dc = disc->find("current");
        if (dc) {
            dl.current.max = {static_cast<int16_t>(dc->get_num("max")), 0};
        }
    }
    return lim;
}

d20::AcTransferLimits parse_ac_limits(const JsonVal& v) {
    d20::AcTransferLimits lim;
    auto* cp = v.find("charge_power");
    if (cp) {
        lim.charge_power.max = {static_cast<int16_t>(cp->get_num("max")), 0};
        lim.charge_power.min = {static_cast<int16_t>(cp->get_num("min")), 0};
    }
    // L2 / L3 / discharge fields omitted for brevity — extend as needed.
    return lim;
}

d20::EvseSetupConfig parse_evse_config(const std::string& json) {

    auto root = MiniJson::parse(json.c_str());
    if (!root || root->kind != JsonVal::Object) {
        throw std::runtime_error("Invalid config JSON");
    }

    d20::EvseSetupConfig cfg;

    cfg.evse_id = root->get_str("evse_id");

    // Energy services
    for (auto& s : root->get_str("energy_services"))
        (void)s;
    if (auto* es = root->find("energy_services")) {
        for (auto& v : es->items) {
            auto s = v.str;
            if (s == "AC")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::AC);
            else if (s == "DC")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::DC);
            else if (s == "AC_BPT")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::AC_BPT);
            else if (s == "DC_BPT")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::DC_BPT);
            else if (s == "AC_DER")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::AC_DER);
            else if (s == "MCS")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::MCS);
            else if (s == "MCS_BPT")
                cfg.supported_energy_services.push_back(dt::ServiceCategory::MCS_BPT);
        }
    }

    // Auth services
    if (auto* as = root->find("auth_services")) {
        for (auto& v : as->items)
            cfg.authorization_services.push_back(v.str == "PnC" ? dt::Authorization::PnC : dt::Authorization::EIM);
    }

    // Control / mobility modes
    if (auto* modes = root->find("control_mobility_modes")) {
        for (auto& m : modes->items) {
            auto cm = m.get_str("control_mode") == "Dynamic" ? dt::ControlMode::Dynamic : dt::ControlMode::Scheduled;
            auto mm = m.get_str("mobility_mode") == "ProvidedBySecc" ? dt::MobilityNeedsMode::ProvidedBySecc
                                                                     : dt::MobilityNeedsMode::ProvidedByEvcc;
            cfg.control_mobility_modes.push_back({cm, mm});
        }
    } else {
        auto cm = root->get_str("control_mode") == "Dynamic" ? dt::ControlMode::Dynamic : dt::ControlMode::Scheduled;
        auto mm = root->get_str("mobility_mode") == "ProvidedBySecc" ? dt::MobilityNeedsMode::ProvidedBySecc
                                                                     : dt::MobilityNeedsMode::ProvidedByEvcc;
        cfg.control_mobility_modes.push_back({cm, mm});
    }

    cfg.enable_certificate_install_service = root->get_bool("cert_install");

    // Limits
    if (auto* dl = root->find("dc_limits"))
        cfg.dc_limits = parse_dc_limits(*dl);
    if (auto* al = root->find("ac_limits"))
        cfg.ac_limits = parse_ac_limits(*al);

    // VAS
    if (auto* vas = root->find("vas_services")) {
        for (auto& v : vas->items)
            cfg.supported_vas_services.push_back(static_cast<uint16_t>(v.num));
    }

    cfg.powersupply_limits = cfg.dc_limits; // default

    return cfg;
}

// ── ControlEvent JSON parsing ─────────────────────────────

d20::ControlEvent parse_control_event(const std::string& json) {
    using d20::ControlEvent;

    auto root = MiniJson::parse(json.c_str());
    if (!root || root->kind != JsonVal::Object)
        throw std::runtime_error("Invalid control event JSON");

    auto kind = root->get_str("kind");

    if (kind == "AuthorizationResponse")
        return d20::AuthorizationResponse(root->get_bool("authorized"));
    if (kind == "CableCheckFinished")
        return d20::CableCheckFinished(root->get_bool("success"));
    if (kind == "StopCharging")
        return d20::StopCharging(root->get_bool("stop"));
    if (kind == "PauseCharging")
        return d20::PauseCharging(root->get_bool("pause"));
    if (kind == "ClosedContactor")
        return d20::ClosedContactor(root->get_bool("closed"));

    if (kind == "PresentVoltageCurrent") {
        return d20::PresentVoltageCurrent{
            static_cast<float>(root->get_num("voltage")),
            static_cast<float>(root->get_num("current")),
        };
    }

    if (kind == "DcTransferLimits") {
        return parse_dc_limits(*root);
    }

    if (kind == "AcTransferLimits") {
        return parse_ac_limits(*root);
    }

    if (kind == "UpdateDynamicModeParameters") {
        d20::UpdateDynamicModeParameters p;
        auto dt = root->get_num("departure_time");
        if (dt > 0)
            p.departure_time = static_cast<uint32_t>(dt);
        p.target_soc = static_cast<uint8_t>(root->get_num("target_soc"));
        p.min_soc = static_cast<uint8_t>(root->get_num("min_soc"));
        return p;
    }

    if (kind == "AcTargetPower") {
        d20::AcTargetPower p;
        auto v = root->get_num("target_active_power");
        if (v != 0)
            p.target_active_power = dt::RationalNumber{static_cast<int16_t>(v), 0};
        v = root->get_num("target_reactive_power");
        if (v != 0)
            p.target_reactive_power = dt::RationalNumber{static_cast<int16_t>(v), 0};
        return p;
    }

    if (kind == "AcPresentPower") {
        d20::AcPresentPower p;
        auto v = root->get_num("present_active_power");
        if (v != 0)
            p.present_active_power = dt::RationalNumber{static_cast<int16_t>(v), 0};
        return p;
    }

    if (kind == "EnergyServices") {
        std::vector<dt::ServiceCategory> svcs;
        if (auto* arr = root->find("services")) {
            for (auto& s : arr->items) {
                if (s.str == "AC")
                    svcs.push_back(dt::ServiceCategory::AC);
                else if (s.str == "DC")
                    svcs.push_back(dt::ServiceCategory::DC);
                else if (s.str == "DC_BPT")
                    svcs.push_back(dt::ServiceCategory::DC_BPT);
            }
        }
        return d20::EnergyServices{std::move(svcs)};
    }

    if (kind == "SupportedVASs") {
        std::vector<uint16_t> ids;
        if (auto* arr = root->find("service_ids")) {
            for (auto& v : arr->items)
                ids.push_back(static_cast<uint16_t>(v.num));
        }
        return d20::SupportedVASs{std::move(ids)};
    }

    throw std::runtime_error("Unknown control event kind: " + kind);
}

// ── JSON event serialization ──────────────────────────────

// Minimal safe JSON string builder.
struct JsonBuf {
    std::string buf;

    void start_obj() {
        buf += '{';
    }
    void end_obj() {
        if (buf.back() == ',')
            buf.pop_back();
        buf += '}';
    }
    void key(const char* k) {
        buf += '"';
        buf += k;
        buf += "\":";
    }
    void str_val(const char* v) {
        buf += '"';
        buf += v;
        buf += '"';
        buf += ',';
    }
    void str_val(const std::string& v) {
        str_val(v.c_str());
    }
    void int_val(int v) {
        buf += std::to_string(v);
        buf += ',';
    }
    void bool_val(bool v) {
        buf += v ? "true," : "false,";
    }
};

void emit_signal(JsonBuf& b, const std::string& session_id, const char* sig) {
    b.start_obj();
    b.key("type");
    b.str_val("signal");
    b.key("session_id");
    b.str_val(session_id);
    b.key("signal");
    b.str_val(sig);
    b.end_obj();
}

// ── Session struct ────────────────────────────────────────

// Implementation of the opaque C type.
struct iso15118_session_t {
    io::PollManager poll_manager;
    std::unique_ptr<io::SdpServer> sdp;
    std::unique_ptr<iso15118::Session> session;
    d20::EvseSetupConfig config;

    iso15118_event_fn event_fn{nullptr};
    void* event_userdata{nullptr};

    std::string session_id_str; // hex string of session ID
    std::string last_error;
    std::mutex push_mutex;

    bool stopped{false};

    void emit(const std::string& json) const {
        if (event_fn)
            event_fn(event_userdata, json.c_str());
    }
};

// ── Callback builder ──────────────────────────────────────

iso15118::session::feedback::Callbacks make_callbacks(iso15118_session_t& s) {
    iso15118::session::feedback::Callbacks cb;

    cb.signal = [&s](iso15118::session::feedback::Signal sig) {
        JsonBuf b;
        const char* name = "UNKNOWN";
        switch (sig) {
        case iso15118::session::feedback::Signal::CHARGE_LOOP_STARTED:
            name = "CHARGE_LOOP_STARTED";
            break;
        case iso15118::session::feedback::Signal::CHARGE_LOOP_FINISHED:
            name = "CHARGE_LOOP_FINISHED";
            break;
        case iso15118::session::feedback::Signal::DLINK_TERMINATE:
            name = "DLINK_TERMINATE";
            break;
        case iso15118::session::feedback::Signal::DLINK_PAUSE:
            name = "DLINK_PAUSE";
            break;
        case iso15118::session::feedback::Signal::DLINK_ERROR:
            name = "DLINK_ERROR";
            break;
        case iso15118::session::feedback::Signal::DC_OPEN_CONTACTOR:
            name = "DC_OPEN_CONTACTOR";
            break;
        case iso15118::session::feedback::Signal::AC_CLOSE_CONTACTOR:
            name = "AC_CLOSE_CONTACTOR";
            break;
        case iso15118::session::feedback::Signal::AC_OPEN_CONTACTOR:
            name = "AC_OPEN_CONTACTOR";
            break;
        case iso15118::session::feedback::Signal::REQUIRE_AUTH_EIM:
            name = "REQUIRE_AUTH_EIM";
            break;
        case iso15118::session::feedback::Signal::START_CABLE_CHECK:
            name = "START_CABLE_CHECK";
            break;
        case iso15118::session::feedback::Signal::SETUP_FINISHED:
            name = "SETUP_FINISHED";
            break;
        case iso15118::session::feedback::Signal::PRE_CHARGE_STARTED:
            name = "PRE_CHARGE_STARTED";
            break;
        }
        emit_signal(b, s.session_id_str, name);
        s.emit(b.buf);
    };

    cb.v2g_message = [&s](const m20::Type& t) {
        JsonBuf b;
        b.start_obj();
        b.key("type");
        b.str_val("v2g_message");
        b.key("session_id");
        b.str_val(s.session_id_str);
        b.key("msg_type");
        b.int_val(static_cast<int>(t));
        b.end_obj();
        s.emit(b.buf);
    };

    cb.evccid = [&s](const std::string& id) {
        JsonBuf b;
        b.start_obj();
        b.key("type");
        b.str_val("evcc_id");
        b.key("session_id");
        b.str_val(s.session_id_str);
        b.key("evcc_id");
        b.str_val(id);
        b.end_obj();
        s.emit(b.buf);
    };

    cb.selected_protocol = [&s](const std::string& proto) {
        JsonBuf b;
        b.start_obj();
        b.key("type");
        b.str_val("selected_protocol");
        b.key("session_id");
        b.str_val(s.session_id_str);
        b.key("protocol");
        b.str_val(proto);
        b.end_obj();
        s.emit(b.buf);
    };

    // dt already declared at namespace scope
    cb.selected_service_parameters = [&s](const d20::SelectedServiceParameters& sp) {
        JsonBuf b;
        b.start_obj();
        b.key("type");
        b.str_val("selected_service");
        b.key("session_id");
        b.str_val(s.session_id_str);
        b.key("energy_service");
        b.int_val(static_cast<int>(sp.selected_energy_service));
        b.key("control_mode");
        b.int_val(static_cast<int>(sp.selected_control_mode));
        b.key("mobility_mode");
        b.int_val(static_cast<int>(sp.selected_mobility_needs_mode));
        b.key("pricing");
        b.int_val(static_cast<int>(sp.selected_pricing));
        b.end_obj();
        s.emit(b.buf);
    };

    // Other callbacks (dc_charge_loop_req, ac_charge_loop_req, etc.)
    // are stubbed for now — extend as needed for full coverage.
    cb.ev_termination = [&s](const std::string& code, const std::string& msg) {
        JsonBuf b;
        b.start_obj();
        b.key("type");
        b.str_val("error");
        b.key("session_id");
        b.str_val(s.session_id_str);
        b.key("code");
        b.str_val(code);
        b.key("message");
        b.str_val(msg);
        b.end_obj();
        s.emit(b.buf);
    };

    return cb;
}

// ── Public C API ──────────────────────────────────────────

static thread_local char g_last_error[256];

extern "C" {

iso15118_session_t* iso15118_session_create(const char* config_json) {
    auto s = std::make_unique<iso15118_session_t>();
    try {
        s->config = parse_evse_config(config_json);
    } catch (const std::exception& e) {
        std::snprintf(g_last_error, sizeof(g_last_error), "Config parse error: %s", e.what());
        return nullptr;
    }

    s->session_id_str = "0000000000000000"; // will be updated after session creation
    return s.release();
}

void iso15118_session_destroy(iso15118_session_t* s) {
    delete s;
}

void iso15118_session_set_callback(iso15118_session_t* s, iso15118_event_fn fn, void* userdata) {
    s->event_fn = fn;
    s->event_userdata = userdata;
}

int iso15118_session_poll(iso15118_session_t* s) {
    if (s->stopped)
        return -1;

    try {
        // Lazy init: create session on first poll
        if (!s->session) {
            auto session_config = d20::SessionConfig(s->config);

            auto conn = std::make_unique<io::ConnectionPlain>(s->poll_manager, "eth0"); // TODO: configurable interface

            auto callbacks = make_callbacks(*s);
            std::optional<d20::PauseContext> pause_ctx{std::nullopt};

            s->session =
                std::make_unique<iso15118::Session>(std::move(conn), std::move(session_config), callbacks, pause_ctx);
        }

        s->poll_manager.poll(50);

        auto& next = s->session->poll();
        if (s->session->is_finished()) {
            s->stopped = true;
            return -1;
        }

        // Return approximate wakeup delay
        return 50;
    } catch (const std::exception& e) {
        std::snprintf(g_last_error, sizeof(g_last_error), "Poll error: %s", e.what());
        s->stopped = true;
        return -1;
    }
}

void iso15118_session_push_event(iso15118_session_t* s, const char* event_json) {
    std::lock_guard<std::mutex> lock(s->push_mutex);
    if (!s->session) {
        std::snprintf(g_last_error, sizeof(g_last_error), "push_event: session not yet created (call poll first)");
        return;
    }
    try {
        auto event = parse_control_event(event_json);
        s->session->push_control_event(event);
    } catch (const std::exception& e) {
        std::snprintf(g_last_error, sizeof(g_last_error), "push_event parse error: %s", e.what());
    }
}

void iso15118_session_close(iso15118_session_t* s) {
    if (s->session)
        s->session->close();
    s->stopped = true;
}

const char* iso15118_last_error(void) {
    return g_last_error;
}

} // extern "C"
