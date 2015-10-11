#include <iostream>
#include <cctype>
#include <cstring>

namespace taoxml {
    static char xml[] = R"(
<a>
<html attr1 attr2>
    <head>
        <meta charset="UTF-8" />
        <style type="text/css"></style>
    </head>
    <body>
        <img id="abc"/>
        <div class="content" data="1 &lt;&lt; 20">
            <span>a &lt; b &gt; c &amp; d &unknown e &#34; f &apos;</span>
        </div>
    </body>
</html>
)";

    enum class TK {
        error=0,tag,attr,assign,value,text,close,close1,close2,eof,next,
    };

    static char* p = nullptr;

    static inline void _skip() {
        while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            p++;
    }

    enum class SC {
        init,tag,
    };

    static SC sc = SC::init;
    static TK tk = TK::error;
    static const char* tt;  // token text
    static char nc; // next char to be used, if not zero

    static void next() {
        if(nc) {
            *p = nc;
            nc = 0;
        }
        if(sc == SC::init) {
            _skip();
            if(*p == '<') {
                p++;
                if(*p == '/') {
                    p++;
                    tt = p;
                    while(::isalnum(*p))
                        p++;
                    if(*p == '>') {
                        *p = '\0';
                        p++;
                        sc = SC::init;
                        tk = TK::close2;
                        return;
                    }
                }
                else if(::isalpha(p[0])) {
                    tt = p;
                    while(::isalnum(*p))
                        p++;
                    if(*p != '\0') {
                        nc = *p;
                        *p = '\0';
                    }
                    sc = SC::tag;
                    tk = TK::tag;
                    return;
                }
            }
            else {
                tt = p;
                while(*p && *p != '<' && *p != '>')
                    p++;
                if(*p != '\0') {
                    nc = *p;
                    *p = '\0';
                }
                tk = *tt ? TK::text
                    : *p ? TK::error
                    : TK::eof;
                return;
            }
        }
        else if(sc == SC::tag) {
            _skip();
            if(::isalpha(*p)) {
                tt = p;
                while(::isalnum(*p))
                    p++;
                if(*p != '\0') {
                    nc = *p;
                    *p = '\0';
                }
                tk = TK::attr;
                return;
            }
            else if(*p == '=') {
                p++;
                tk = TK::assign;
                return;
            }
            else if(*p == '\'' || *p == '\"') {
                const char c = *p++;
                tt = p;
                while(*p && *p != c)
                    p++;
                if(*p == c) {
                    *p = '\0';
                    p++;
                    tk = TK::value;
                    return;
                }
            }
            else if(*p == '/') {
                p++;
                if(*p == '>') {
                    p++;
                    sc = SC::init;
                    tk = TK::close1;
                    return;
                }
            }
            else if(*p == '>') {
                p++;
                sc = SC::init;
                tk = TK::close;
                return;
            }
        }

        if(!*p) {
            tk = TK::eof;
            return;
        }

        tk = TK::error;
        return;
    }

    static void parse(char* xml) {
        p = xml;
        for(;;) {
            next();
            if(tk == TK::tag) {
                const char* tag_name = tt;
                std::cout << tt;
                next();
                while(tk == TK::attr) {
                    std::cout << " " << tt;
                    next();
                    if(tk == TK::assign) {
                        std::cout << "=\"";
                        next();
                        if(tk == TK::value) {
                            std::cout << tt << "\"";
                            next();
                            continue;
                        }

                        throw "value expected after assignment.";
                    }
                }

                if(tk == TK::close) {
                    std::cout << "\n";
                    for(;;) {
                        parse(p);
                        if(tk == TK::next)
                            continue;

                        else if(tk == TK::close2) {
                            if(::strncmp(tag_name, tt, ::strlen(tt)) != 0)
                                throw "mismatched opening tag and closing tag.";

                            tk = TK::next;
                            std::cout << tt;
                            std::cout << "\n";
                            return;
                        }

                        if(tk == TK::eof) throw "premature eof, expecting closing tag.";
                        else throw "expecting closing tag.";
                    }
                }
                else if(tk == TK::close1) {
                    tk = TK::next;
                    std::cout << "\n";
                    return;
                }

                throw "unexpected following token for open tag.";
            }
            else if(tk == TK::text) {
                tk = TK::next;
                std::cout << "<!CDATA[" << tt << "]]\n";
                return;
            }
            else if(tk == TK::close2) {
                return;
            }
            else if(tk == TK::eof) {
                return;
            }

            throw "unexpected token while calling parse.";
        }
    }
}

int main() {
    try {
        taoxml::parse(taoxml::xml);
    }
    catch(const char* e) {
        std::cout << "error: " << e << std::endl;
    }
    return 0;
}
