#include "do_cpprest_uri_builder.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
namespace cpprest_web
{

static const cpprest_utils::string_t oneSlash = _XPLATSTR("/");

uri_builder& uri_builder::append_path(const cpprest_utils::string_t& toAppend, bool do_encode)
{
    if (!toAppend.empty() && toAppend != oneSlash)
    {
        auto& thisPath = m_uri.m_path;
        if (&thisPath == &toAppend)
        {
            auto appendCopy = toAppend;
            return append_path(appendCopy, do_encode);
        }

        if (thisPath.empty() || thisPath == oneSlash)
        {
            thisPath.clear();
            if (toAppend.front() != _XPLATSTR('/'))
            {
                thisPath.push_back(_XPLATSTR('/'));
            }
        }
        else if (thisPath.back() == _XPLATSTR('/') && toAppend.front() == _XPLATSTR('/'))
        {
            thisPath.pop_back();
        }
        else if (thisPath.back() != _XPLATSTR('/') && toAppend.front() != _XPLATSTR('/'))
        {
            thisPath.push_back(_XPLATSTR('/'));
        }
        else
        {
            // Only one slash.
        }

        if (do_encode)
        {
            thisPath.append(uri::encode_uri(toAppend, uri::components::path));
        }
        else
        {
            thisPath.append(toAppend);
        }
    }

    return *this;
}

uri_builder& uri_builder::append_path_raw(const cpprest_utils::string_t& toAppend, bool do_encode)
{
    if (!toAppend.empty())
    {
        auto& thisPath = m_uri.m_path;
        if (&thisPath == &toAppend)
        {
            auto appendCopy = toAppend;
            return append_path_raw(appendCopy, do_encode);
        }

        if (thisPath != oneSlash)
        {
            thisPath.push_back(_XPLATSTR('/'));
        }

        if (do_encode)
        {
            thisPath.append(uri::encode_uri(toAppend, uri::components::path));
        }
        else
        {
            thisPath.append(toAppend);
        }
    }

    return *this;
}

uri_builder& uri_builder::append_query(const cpprest_utils::string_t& toAppend, bool do_encode)
{
    if (!toAppend.empty())
    {
        auto& thisQuery = m_uri.m_query;
        if (&thisQuery == &toAppend)
        {
            auto appendCopy = toAppend;
            return append_query(appendCopy, do_encode);
        }

        if (thisQuery.empty())
        {
            thisQuery.clear();
        }
        else if (thisQuery.back() == _XPLATSTR('&') && toAppend.front() == _XPLATSTR('&'))
        {
            thisQuery.pop_back();
        }
        else if (thisQuery.back() != _XPLATSTR('&') && toAppend.front() != _XPLATSTR('&'))
        {
            thisQuery.push_back(_XPLATSTR('&'));
        }
        else
        {
            // Only one ampersand.
        }

        if (do_encode)
        {
            thisQuery.append(uri::encode_uri(toAppend, uri::components::query));
        }
        else
        {
            thisQuery.append(toAppend);
        }
    }

    return *this;
}

uri_builder& uri_builder::set_port(const cpprest_utils::string_t& port)
{
    cpprest_utils::istringstream_t portStream(port);
    portStream.imbue(std::locale::classic());
    int port_tmp;
    portStream >> port_tmp;
    if (portStream.fail() || portStream.bad())
    {
        throw std::invalid_argument("invalid port argument, must be non empty string containing integer value");
    }
    m_uri.m_port = port_tmp;
    return *this;
}

uri_builder& uri_builder::append(const uri& relative_uri)
{
    append_path(relative_uri.path());
    append_query(relative_uri.query());
    this->set_fragment(this->fragment() + relative_uri.fragment());
    return *this;
}

cpprest_utils::string_t uri_builder::to_string() const { return to_uri().to_string(); }

uri uri_builder::to_uri() const { return uri(m_uri); }

bool uri_builder::is_valid() { return uri::validate(m_uri.join()); }

void uri_builder::append_query_encode_impl(const cpprest_utils::string_t& name, const cpprest_utils::utf8string& value)
{
    cpprest_utils::string_t encodedQuery = uri::encode_query_impl(cpprest_utils::to_utf8string(name));
    encodedQuery.push_back(_XPLATSTR('='));
    encodedQuery.append(uri::encode_query_impl(value));

    // The query key value pair was already encoded by us or the user separately.
    append_query(encodedQuery, false);
}

void uri_builder::append_query_no_encode_impl(const cpprest_utils::string_t& name, const cpprest_utils::string_t& value)
{
    append_query(name + _XPLATSTR("=") + value, false);
}

} // namespace cpprestutils
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
