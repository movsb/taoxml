#include <iostream>
#include <string>
#include <vector>
#include <map>

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
        attr,           // attribute name
        assign,         // =
        value,          // "xxx", attribute value
    };

    enum class read_type {
        text,
        ws,
        tk,
    };

    enum class node_type {
        node,
        text,
    };

    class node_node_t;
    class text_node_t;

    class node_base_t {
    public:
        node_type type() const {
            return _type;
        }

        virtual node_node_t* to_node() = 0;
        virtual text_node_t* to_text() = 0;

    protected:
        node_base_t(node_type type)
            : _type(type)
        {}

    protected:
        node_type _type;
    };

    class node_node_t : public node_base_t {
        typedef std::vector<node_base_t*>           children_t;
        typedef std::map<std::string, std::string>  attr_t;

    public:
        node_node_t()
            : node_base_t(node_type::node)
            , _parent(nullptr)
            , _prev(nullptr)
            , _next(nullptr)
            , _cindex(-1)
        {}

        virtual node_node_t* to_node() override {
            return this;
        }

        virtual text_node_t* to_text() override {
            return nullptr;
        }

    public:
        void set_tag(const std::string& tag) {
            _tag = tag;
        }

        const std::string& tag() const {
            return _tag;
        }

        void add_attr(const std::string& key, const std::string& val) {
            _attrs[key] = val;
        }

        size_t size_attr() const {
            return _attrs.size();
        }

        attr_t::const_iterator attr(int i) const {
            std::string t;
            if(0 <= i && i < (int)size_attr()) {
                int x = 0;
                auto ai = _attrs.cbegin();
                for(;  x < i;) {
                    ai = ai++;
                    x++;
                }

                return ai;
            }

            return _attrs.cend();
        }

        void add_child(node_base_t* node) {
            _children.push_back(node);
        }

    public:
        node_node_t*    parent() const {
            return _parent;
        }

        node_base_t*    big() const {
            return _prev;
        }

        node_base_t*    little() const{
            return _next;
        }

        size_t size() const {
            return _children.size();
        }

        node_base_t* operator[](int i) {
            return _children[i];
        }

        node_node_t& select(const char* tag) {
            _search = tag;
            _cindex = -1;

            return *this;
        }

        node_base_t* next() {
            for (int i = _cindex+1, n = (int)_children.size(); i < n; i++) {
                if (!_search.size() // search for next text node
                    || _children[i]->type() == node_type::node && _children[i]->to_node()->tag() == _search // search for next tagged node
                    )
                {
                    _cindex = i;
                    return _children[i];
                }
            }

            return nullptr;
        }

        node_base_t* prev() {
            for (int i = _cindex - 1, n = (int)_children.size(); i >= 0; i--) {
                if (!_search.size() // search for next text node
                    || _children[i]->type() == node_type::node && _children[i]->to_node()->tag() == _search // search for next tagged node
                    )
                {
                    _cindex = i;
                    return _children[i];
                }
            }

            return nullptr;
        }

    protected:
        node_base_t*    _prev;
        node_base_t*    _next;
        node_node_t*    _parent;
        children_t      _children;
        int             _cindex;
        attr_t          _attrs;
        std::string     _tag;
        std::string     _search;
    };

    class text_node_t : public node_base_t {
    public:
        text_node_t()
            : node_base_t(node_type::text)
        {}

        virtual node_node_t* to_node() override {
            return nullptr;
        }

        virtual text_node_t* to_text() override {
            return this;
        }

        void set_value(const std::string& val) {
            _value = val;
        }

        const std::string&  value() const {
            return _value;
        }

    protected:
        std::string     _value;
    };

    class find_node_t {
    public:
        find_node_t(node_base_t* node)
            : _base(node)
        {}

        find_node_t operator[](const char* tag) {
            if (_base && _base->type() == node_type::node) {
                return _base->to_node()->select(tag).next();
            }

            return nullptr;
        }

        // convert to node_node_t
        node_node_t*    to_node() const {
            return _base ? _base->to_node() : nullptr;
        }

        // tagged nodes are always node_node_t, so this function is used to
        // convert the node's first node to text_node_t
        text_node_t*    to_text() const {
            if(_base && _base->to_node()) {
                auto next = _base->to_node()->select("").next();
                if(next) return next->to_text();
            }

            return nullptr;
        }

    protected:
        node_base_t*    _base;
    };

    class taoxml_t {
    public:
        taoxml_t() {}
        ~taoxml_t() {}

        node_node_t* root() {
            return &_root;
        }

        bool parse(const char* xml) {
            _xml = xml;
            _p = _xml;
            _line = 0;
            _char = 0;

            token tk;
            node_base_t* node = nullptr;

            while(((int)(tk = _parse_node(&node)) || 1) && (tk == token::close || tk == token::close2)) {
                // text nodes are not allowed on root
                if(node && node->type() == node_type::node)
                    _root.add_child(node);
            }

            return true;
        }

    protected:  // called by public
        token _parse_node(node_base_t** node) {
            token tk;

            tk = _token(false);

            if(tk == token::close)
                return token::close3;

            if(tk == token::open) {
                node_node_t* newnode = new node_node_t;

                newnode->set_tag(_tk);

                while(((int)(tk = _token(true)) || 1) && (tk == token::attr || tk == token::space)) {
                    if(tk == token::space)
                        continue;

                    std::string key(_tk);

                    tk = _token(true);
                    if(tk == token::space)
                        tk = _token(true);
                    if(tk != token::assign)
                        return token::error;

                    tk = _token(true);
                    if(tk == token::space)
                        tk = _token(true);
                    if(tk != token::value)
                        return token::error;

                    newnode->add_attr(key, _tk);
                }

                if(tk == token::close2) {
                    *node = newnode;
                    return token::close2;
                }
                else if(tk == token::open2) {
                    node_base_t* cnode = nullptr;
                    while (((int)(tk = _parse_node(&cnode)) || 1) && tk != token::close3 && tk != token::error)
                        if(cnode)
                            newnode->add_child(cnode);

                    if(tk == token::close3) {
                        *node = newnode;
                        return token::close;
                    }
                }
            }
            else if(tk == token::text) { // text
                if (!_is_empty()) {
                    text_node_t* newnode = new text_node_t;
                    newnode->set_value(_tk);
                    *node = newnode;
                }
                else {
                    *node = nullptr;
                }
                    
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

    protected:  // vars
        node_node_t     _root;      // root node
        const char*     _xml;       // raw xml string
        const char*     _p;         // current pointer to content
        std::string     _tk;        // current token
        int             _line;      // current line number
        int             _char;      // current char position
    };
}

std::ostream& operator <<(std::ostream& os, taoxml::text_node_t& text) {
    os << "<![CDATA[" << text.value() << "]]>\n";
    return os;
}

std::ostream& operator <<(std::ostream& os, taoxml::node_node_t& node) {
    // print open tag
    os << '<' << node.tag();

    // print tag attrs
    if(node.size_attr()) {
        for(int i = 0; i < (int)node.size_attr(); i++) {
            auto ai = node.attr(i);
            std::cout << ' ' << ai->first << "=\"" << ai->second << "\"";
        }
    }

    // if has children
    if(node.size()) {
        // print close open tag
        os << ">\n";
        
        // recursively print children
        node.select("");
        while(auto next = node.next()) {
            if(next->type() == taoxml::node_type::node)
                os << *next->to_node();
            else if(next->type() == taoxml::node_type::text)
                os << *next->to_text();
        }

        // print closing tag
        os << "</" << node.tag() << ">\n";
    }
    else {
        os << "/>\n";
    }

    return os;
}

void dump_node_tree(taoxml::node_node_t* node) {
    if(!node) return;

    std::cout << *node;
}

int main() {
    const char* xml = R"(
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

    taoxml::taoxml_t tx;
    tx.parse(xml);
    taoxml::text_node_t* span = taoxml::find_node_t(tx.root())["html"]["body"]["div"]["span"].to_text();
    dump_node_tree(tx.root()->select("").next()->to_node());

    return 0;
}
