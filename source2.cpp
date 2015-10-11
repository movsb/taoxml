#include <iostream>
#include <string>
#include <cctype>

namespace taoxml {
    static char xml[] = R"(
<html>
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

    static void _skip() {
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

    static bool next() {
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
                    while(::isalpha(*p))
                        p++;
                    if(*p == '>') {
                        *p = '\0';
                        p++;
                        sc = SC::init;
                        tk = TK::close2;
                        return true;
                    }
                }
                else if(::isalpha(p[0])) {
                    tt = p;
                    while(::isalpha(*p))
                        p++;
                    if(*p != '\0') {
                        nc = *p;
                        *p = '\0';
                    }
                    sc = SC::tag;
                    tk = TK::tag;
                    return true;
                }
            }
            else {
                tt = p;
                while(*p && *p != '<')
                    p++;
                if(*p != '\0') {
                    nc = *p;
                    *p = '\0';
                }
                tk = TK::text;
                return true;
            }
        }
        else if(sc == SC::tag) {
            _skip();
            if(::isalpha(*p)) {
                tt = p;
                while(::isalpha(*p))
                    p++;
                if(*p == '=')
                    nc = *p;
                if(*p != '\0') {
                    *p = '\0';
                }
                tk = TK::attr;
                return true;
            }
            else if(*p == '=') {
                p++;
                tk = TK::assign;
                return true;
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
                    return true;
                }
            }
            else if(*p == '/') {
                p++;
                if(*p == '>') {
                    p++;
                    sc = SC::init;
                    tk = TK::close1;
                    return true;
                }
            }
            else if(*p == '>') {
                p++;
                sc = SC::init;
                tk = TK::close;
                return true;
            }
        }

        if(!*p) {
            tk = TK::eof;
            return true;
        }

        tk = TK::error;
        return false;
    }

    static void parse(char* xml) {
        p = xml;
        while(next()) {
            if(tk == TK::tag) {
                std::cout << tt << " ";
                next();
                while(tk == TK::attr) {
                    std::cout << tt;
                    next();
                    if(tk == TK::assign) {
                        std::cout << "=";
                        next();
                        if(tk == TK::value) {
                            std::cout << tt;
                            next();
                            continue;
                        }
                    }
                }
                if(tk == TK::close) {
                    std::cout << "\n";
                    for(;;) {
                        parse(p);
                        if(tk == TK::next)
                            continue;

                        if(tk == TK::close2) {
                            tk = TK::next;
                            std::cout << tt;
                            std::cout << "\n";
                            return;
                        }
                    }
                }
                else if(tk == TK::close1) {
                    std::cout << "\n";
                    return;
                }
                else if(tk == TK::close2) {
                    std::cout << "\n";
                    tk = TK::next;
                    return;
                }
            }
            else if(tk == TK::text) {
                std::cout << tt;
                std::cout << "\n";
                return;
            }
            else if(tk == TK::close2) {
                return;
            }
            else if(tk == TK::eof) {
                return;
            }

            return;
        }
    }
}

int main() {
    taoxml::parse(taoxml::xml);
    return 0;
}
