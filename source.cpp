#include <iostream>
#include <string>

namespace taoxml {
    enum class token {
        error,          // error
        open,           // <html
        open2,          // >
        close,          // </html>, normal closing
        close2,         // <xxx  ---> /> <---, self closing
        close3,         // </xxx>, parent closing
        space,          // space
        text,           // text
        attr,          // attribute name
        assign,         // =
        value,          // "xxx", attribute value
    };

    enum class read_type {
        text,
        ws,
        tk,
    };

    class taoxml_t {
    public:
        taoxml_t() {}
        ~taoxml_t() {}

        bool parse(const char* xml) {
            _xml = xml;
            _p = _xml;
            _line = 0;
            _char = 0;
            _indent = 0;

            token tk;

            while(((int)(tk = _parse_node()) || 1) && (tk == token::close || tk == token::close2)) {
                std::cout << "noding ...\n";
            }

            return true;
        }

    protected:  // called by public
        token _parse_node() {
            token tk;

            tk = _token(false);

            if(tk == token::close)
                return token::close3;

            if(tk == token::open) {
                std::cout << *this << "<" << _tk;

                while(((int)(tk = _token(true)) || 1) && (tk == token::attr || tk == token::space)) {
                    if(tk == token::space)
                        continue;

                    std::cout << " " << _tk;

                    tk = _token(true);
                    if(tk == token::space)
                        tk = _token(true);
                    if(tk != token::assign)
                        return token::error;

                    std::cout << "=";

                    tk = _token(true);
                    if(tk == token::space)
                        tk = _token(true);
                    if(tk != token::value)
                        return token::error;

                    std::cout << "\"" << _tk << "\"";
                }

                if(tk == token::close2) {
                    std::cout << " />\n";
                    return tk;
                }
                else if(tk == token::open2) {
                    std::cout << ">\n";

                    _indent += 4;
                    while(((int)(tk = _parse_node()) || 1) && tk != token::close3 && tk != token::error)
                        ;
                    _indent -= 4;

                    if(tk == token::close3) {
                        std::cout << *this << "</" << _tk << ">\n";
                        return token::close;
                    }
                }
            }
            else if(tk == token::text) { // text
                if(!_is_empty())
                    std::cout << *this << _tk << "\n";
                return token::close2;
            }

            return token::error;
        }

        token _token(bool intag) {
            if(intag) {
                if(_is_ws()) {
                    if(!_read_token(read_type::ws))
                        return token::error;
                    return token::space;
                }
                else if(_is_alnum()) {
                    if (!_read_token(read_type::tk))
                        return token::error;
                    return token::attr;
                }
                else if(*_p == '=') {
                    ++_p;
                    return token::assign;
                }
                else if(*_p == '\'' || *_p == '"') {
                    char c = *_p++;
                    _tk = "";
                    while(*_p && *_p != c) {
                        if(*_p == '&') {
                            if(!_read_entity())
                                return token::error;
                        }
                        else {
                            _tk += *_p++;
                        }
                    }

                    if(*_p != c)
                        return token::error;

                    ++_p;

                    return token::value;
                }
                else if(*_p == '/') { // close2 
                    if(*++_p != '>')
                        return token::error;
                    ++_p;
                    return token::close2;
                }
                else if(*_p == '>') {
                    ++_p;
                    
                    return token::open2;
                }
            }
            else {
                if(*_p == '<') { // open, close, comment
                    if(!_valid_ptr(true))
                        return token::error;

                    if(_is_alnum()) { // open
                        if(!_read_token(read_type::tk))
                            return token::error;
                        return token::open;
                    } else if(*_p == '/') { // close
                        if(!_valid_ptr(true))
                            return token::error;

                        if(!_read_token(read_type::tk))
                            return token::error;

                        if(!_valid_ptr())
                            return token::error;

                        if(*_p++ != '>')
                            return token::error;

                        return token::close;
                    }
                } else if(_is_text()) {
                    if(!_read_token(read_type::text))
                        return token::error;
                    return token::text;
                }
            }

            return token::error;
        }

    protected:  // aux, called by protected

        // support only `&nbsp;`, `&quot;(&#34;)`, `&apos;(&#39;)`, `&lt;`, `&gt;`, `&amp;`
        bool _read_entity() {
            static struct entities_t {
                const char* name;
                const char* got;
            } entities[] = {
                {"nbsp", " "},
                {"quot", "\""}, {"#34", "\""},
                {"apos", "'"}, {"#39", "'"},
                {"lt", "<"},
                {"gt", ">"},
                {"amp", "&"},
            };

            if(*_p != '&') return false;

            for(int i = 0, n = sizeof(entities) / sizeof(entities[0]); i < n; i++) {
                auto name = entities[i].name;
                auto len = strlen(name);
                if(_p[1] == name[0] && strncmp(&_p[1], name, len) == 0) {
                    if(_p[len + 1] == ';') {
                        _tk += entities[i].got;
                        _p += len + 1 + 1;
                        return true;
                    }
                }
            }

            // ignores unrecognized entity
            _tk += '&';
            ++_p;
            return true;
        }

        // tokens are: text node, identifier, white spaces
        int _read_token(read_type type) {
            int n = 0;
            _tk = "";

            bool (taoxml_t::*func)();

            switch (type){
            case read_type::text:   func = &taoxml_t::_is_text;    break;
            case read_type::tk:     func = &taoxml_t::_is_alnum;   break;
            case read_type::ws:     func = &taoxml_t::_is_ws;      break;
            default: return 0;
            }

            while ((this->*func)()) {
                if(*_p == '&') {
                    if(!_read_entity())
                        return 0;
                    ++n;
                }
                else {
                    _tk += *_p++;
                    ++n;
                }
            }

            return n;
        }

        inline bool _valid_ptr(bool inc = false) {
            if(inc) ++_p;
            return !!*_p;
        }

        inline bool _valid_offset(int offset) {
            return !!_p[offset];
        }

        inline bool _is_alnum() {
            return 0
                || *_p >= 'a' && *_p <= 'z'
                || *_p >= 'A' && *_p <= 'Z'
                || *_p >= '0' && *_p <= '9'
                ;
        }
        inline bool _is_text() {
            return *_p != '\0'
                && *_p != '<'
                && *_p != '>'
                ;
        }

        inline bool _is_ws() {
            return 0
                || *_p == ' '
                || *_p == '\n'
                || *_p == '\t'
                || *_p == '\r'
                ;
        }

        inline bool _is_empty() {
            const char* p = _tk.c_str();
            while(*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')
                p++;

            return *p == '\0';
        }

        friend std::ostream& operator<<(std::ostream& os, const taoxml_t& tx) {
            os << std::string(tx._indent, ' ');
            return os;
        }

    protected:  // vars
        const char*     _xml;       // raw xml string
        const char*     _p;         // current pointer to content
        std::string     _tk;        // current token
        int             _line;      // current line number
        int             _char;      // current char position
        int             _indent;    // indent spaces
    };
}

int main() {
    const char* xml = R"(
<html>
    <h2>title<a></a></h2>
    <head>
        <meta charset="UTF-8" />
        <style type="text/css"></style>
    </head>
    <body>
        <div />
        <img id="abc"/>
        <div class="content" data="1 &lt;&lt; 20">
            <span>a &lt; b &gt; c &amp; d &unknown e &#34; f &apos;</span>
        </div>
    </body>
</html>
)";

    taoxml::taoxml_t tx;
    tx.parse(xml);

    return 0;
}
