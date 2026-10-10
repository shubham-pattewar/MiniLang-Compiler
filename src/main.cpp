#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// MiniLang: a small educational compiler implemented in C++17.
// Pipeline: lexer -> parser/AST -> semantic checks -> IR -> optimizer -> VM.

struct LangError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

enum class TokenType
{
    End,
    Identifier,
    Number,
    Let,
    Print,
    If,
    Else,
    While,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Semicolon,
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};

struct Token
{
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

static std::string tokenName(TokenType t)
{
    switch (t)
    {
    case TokenType::End:
        return "EOF";
    case TokenType::Identifier:
        return "IDENTIFIER";
    case TokenType::Number:
        return "NUMBER";
    case TokenType::Let:
        return "LET";
    case TokenType::Print:
        return "PRINT";
    case TokenType::If:
        return "IF";
    case TokenType::Else:
        return "ELSE";
    case TokenType::While:
        return "WHILE";
    case TokenType::LeftParen:
        return "LEFT_PAREN";
    case TokenType::RightParen:
        return "RIGHT_PAREN";
    case TokenType::LeftBrace:
        return "LEFT_BRACE";
    case TokenType::RightBrace:
        return "RIGHT_BRACE";
    case TokenType::Semicolon:
        return "SEMICOLON";
    case TokenType::Plus:
        return "PLUS";
    case TokenType::Minus:
        return "MINUS";
    case TokenType::Star:
        return "STAR";
    case TokenType::Slash:
        return "SLASH";
    case TokenType::Equal:
        return "EQUAL";
    case TokenType::EqualEqual:
        return "EQUAL_EQUAL";
    case TokenType::BangEqual:
        return "BANG_EQUAL";
    case TokenType::Less:
        return "LESS";
    case TokenType::LessEqual:
        return "LESS_EQUAL";
    case TokenType::Greater:
        return "GREATER";
    case TokenType::GreaterEqual:
        return "GREATER_EQUAL";
    }
    return "UNKNOWN";
}

class Lexer
{
    const std::string source;
    size_t pos = 0;
    int line = 1, column = 1;
    char peek(size_t ahead = 0) const { return pos + ahead < source.size() ? source[pos + ahead] : '\0'; }
    char advance()
    {
        char c = peek();
        if (c)
        {
            ++pos;
            if (c == '\n')
            {
                ++line;
                column = 1;
            }
            else
                ++column;
        }
        return c;
    }
    bool match(char expected)
    {
        if (peek() != expected)
            return false;
        advance();
        return true;
    }
    void skipSpaceAndComments()
    {
        while (true)
        {
            if (std::isspace(static_cast<unsigned char>(peek())))
            {
                advance();
                continue;
            }
            if (peek() == '/' && peek(1) == '/')
            {
                while (peek() && peek() != '\n')
                    advance();
                continue;
            }
            break;
        }
    }

public:
    explicit Lexer(std::string text) : source(std::move(text)) {}
    std::vector<Token> scan()
    {
        std::vector<Token> tokens;
        while (true)
        {
            skipSpaceAndComments();
            int startLine = line, startColumn = column;
            char c = peek();
            if (!c)
            {
                tokens.push_back({TokenType::End, "", line, column});
                break;
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
            {
                std::string word;
                while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
                    word += advance();
                TokenType type = TokenType::Identifier;
                if (word == "let")
                    type = TokenType::Let;
                else if (word == "print")
                    type = TokenType::Print;
                else if (word == "if")
                    type = TokenType::If;
                else if (word == "else")
                    type = TokenType::Else;
                else if (word == "while")
                    type = TokenType::While;
                tokens.push_back({type, word, startLine, startColumn});
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(c)))
            {
                std::string number;
                while (std::isdigit(static_cast<unsigned char>(peek())))
                    number += advance();
                tokens.push_back({TokenType::Number, number, startLine, startColumn});
                continue;
            }
            advance();
            TokenType type;
            switch (c)
            {
            case '(':
                type = TokenType::LeftParen;
                break;
            case ')':
                type = TokenType::RightParen;
                break;
            case '{':
                type = TokenType::LeftBrace;
                break;
            case '}':
                type = TokenType::RightBrace;
                break;
            case ';':
                type = TokenType::Semicolon;
                break;
            case '+':
                type = TokenType::Plus;
                break;
            case '-':
                type = TokenType::Minus;
                break;
            case '*':
                type = TokenType::Star;
                break;
            case '/':
                type = TokenType::Slash;
                break;
            case '=':
                type = match('=') ? TokenType::EqualEqual : TokenType::Equal;
                break;
            case '!':
                if (match('='))
                    type = TokenType::BangEqual;
                else
                    throw LangError("Unexpected '!' at " + std::to_string(startLine) + ":" + std::to_string(startColumn) + "; did you mean '!='?");
                break;
            case '<':
                type = match('=') ? TokenType::LessEqual : TokenType::Less;
                break;
            case '>':
                type = match('=') ? TokenType::GreaterEqual : TokenType::Greater;
                break;
            default:
                throw LangError("Unexpected character '" + std::string(1, c) + "' at " + std::to_string(startLine) + ":" + std::to_string(startColumn));
            }
            tokens.push_back({type, std::string(1, c), startLine, startColumn});
            if (type == TokenType::EqualEqual)
                tokens.back().lexeme = "==";
            if (type == TokenType::BangEqual)
                tokens.back().lexeme = "!=";
            if (type == TokenType::LessEqual)
                tokens.back().lexeme = "<=";
            if (type == TokenType::GreaterEqual)
                tokens.back().lexeme = ">=";
        }
        return tokens;
    }
};

struct Expr
{
    enum class Kind
    {
        Number,
        Variable,
        Unary,
        Binary
    } kind;
    long long number = 0;
    std::string text;
    std::unique_ptr<Expr> left, right;
    explicit Expr(long long n) : kind(Kind::Number), number(n) {}
    Expr(Kind k, std::string t, std::unique_ptr<Expr> l = nullptr, std::unique_ptr<Expr> r = nullptr)
        : kind(k), text(std::move(t)), left(std::move(l)), right(std::move(r)) {}
};
struct Stmt
{
    enum class Kind
    {
        Let,
        Assign,
        Print,
        If,
        While,
        Block
    } kind;
    std::string name;
    std::unique_ptr<Expr> expr;
    std::vector<std::unique_ptr<Stmt>> body, alternative;
    Stmt(Kind k, std::string n = "", std::unique_ptr<Expr> e = nullptr) : kind(k), name(std::move(n)), expr(std::move(e)) {}
};

class Parser
{
    const std::vector<Token> &tokens;
    size_t current = 0;
    const Token &peek() const { return tokens[current]; }
    const Token &previous() const { return tokens[current - 1]; }
    bool check(TokenType t) const { return peek().type == t; }
    bool match(TokenType t)
    {
        if (!check(t))
            return false;
        ++current;
        return true;
    }
    const Token &consume(TokenType t, const std::string &message)
    {
        if (check(t))
            return tokens[current++];
        throw LangError("Syntax error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": " + message + "; got " + tokenName(peek().type));
    }
    std::unique_ptr<Stmt> statement()
    {
        if (match(TokenType::Let))
        {
            auto id = consume(TokenType::Identifier, "expected variable name after 'let'");
            consume(TokenType::Equal, "expected '=' after variable name");
            auto e = expression();
            consume(TokenType::Semicolon, "expected ';' after declaration");
            return std::make_unique<Stmt>(Stmt::Kind::Let, id.lexeme, std::move(e));
        }
        if (match(TokenType::Print))
        {
            consume(TokenType::LeftParen, "expected '(' after 'print'");
            auto e = expression();
            consume(TokenType::RightParen, "expected ')' after print expression");
            consume(TokenType::Semicolon, "expected ';' after print");
            return std::make_unique<Stmt>(Stmt::Kind::Print, "", std::move(e));
        }
        if (match(TokenType::If))
        {
            consume(TokenType::LeftParen, "expected '(' after 'if'");
            auto cond = expression();
            consume(TokenType::RightParen, "expected ')' after condition");
            auto s = std::make_unique<Stmt>(Stmt::Kind::If, "", std::move(cond));
            s->body = blockBody();
            if (match(TokenType::Else))
                s->alternative = blockBody();
            return s;
        }
        if (match(TokenType::While))
        {
            consume(TokenType::LeftParen, "expected '(' after 'while'");
            auto cond = expression();
            consume(TokenType::RightParen, "expected ')' after condition");
            auto s = std::make_unique<Stmt>(Stmt::Kind::While, "", std::move(cond));
            s->body = blockBody();
            return s;
        }
        if (match(TokenType::LeftBrace))
        {
            auto s = std::make_unique<Stmt>(Stmt::Kind::Block);
            while (!check(TokenType::RightBrace) && !check(TokenType::End))
                s->body.push_back(statement());
            consume(TokenType::RightBrace, "expected '}' after block");
            return s;
        }
        if (check(TokenType::Identifier) && tokens[current + 1].type == TokenType::Equal)
        {
            std::string name = tokens[current++].lexeme;
            ++current;
            auto e = expression();
            consume(TokenType::Semicolon, "expected ';' after assignment");
            return std::make_unique<Stmt>(Stmt::Kind::Assign, name, std::move(e));
        }
        throw LangError("Syntax error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": expected a statement");
    }
    std::vector<std::unique_ptr<Stmt>> blockBody()
    {
        consume(TokenType::LeftBrace, "expected '{' before block");
        std::vector<std::unique_ptr<Stmt>> result;
        while (!check(TokenType::RightBrace) && !check(TokenType::End))
            result.push_back(statement());
        consume(TokenType::RightBrace, "expected '}' after block");
        return result;
    }
    std::unique_ptr<Expr> expression() { return equality(); }
    std::unique_ptr<Expr> equality()
    {
        auto e = comparison();
        while (match(TokenType::EqualEqual) || match(TokenType::BangEqual))
        {
            std::string op = previous().lexeme;
            auto r = comparison();
            e = std::make_unique<Expr>(Expr::Kind::Binary, op, std::move(e), std::move(r));
        }
        return e;
    }
    std::unique_ptr<Expr> comparison()
    {
        auto e = term();
        while (match(TokenType::Less) || match(TokenType::LessEqual) || match(TokenType::Greater) || match(TokenType::GreaterEqual))
        {
            std::string op = previous().lexeme;
            auto r = term();
            e = std::make_unique<Expr>(Expr::Kind::Binary, op, std::move(e), std::move(r));
        }
        return e;
    }
    std::unique_ptr<Expr> term()
    {
        auto e = factor();
        while (match(TokenType::Plus) || match(TokenType::Minus))
        {
            std::string op = previous().lexeme;
            auto r = factor();
            e = std::make_unique<Expr>(Expr::Kind::Binary, op, std::move(e), std::move(r));
        }
        return e;
    }
    std::unique_ptr<Expr> factor()
    {
        auto e = unary();
        while (match(TokenType::Star) || match(TokenType::Slash))
        {
            std::string op = previous().lexeme;
            auto r = unary();
            e = std::make_unique<Expr>(Expr::Kind::Binary, op, std::move(e), std::move(r));
        }
        return e;
    }
    std::unique_ptr<Expr> unary()
    {
        if (match(TokenType::Minus))
        {
            std::string op = previous().lexeme;
            return std::make_unique<Expr>(Expr::Kind::Unary, op, unary());
        }
        return primary();
    }
    std::unique_ptr<Expr> primary()
    {
        if (match(TokenType::Number))
        {
            try
            {
                return std::make_unique<Expr>(std::stoll(previous().lexeme));
            }
            catch (...)
            {
                throw LangError("Integer literal is too large: " + previous().lexeme);
            }
        }
        if (match(TokenType::Identifier))
            return std::make_unique<Expr>(Expr::Kind::Variable, previous().lexeme);
        if (match(TokenType::LeftParen))
        {
            auto e = expression();
            consume(TokenType::RightParen, "expected ')' after expression");
            return e;
        }
        throw LangError("Syntax error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": expected an expression");
    }

public:
    explicit Parser(const std::vector<Token> &ts) : tokens(ts) {}
    std::vector<std::unique_ptr<Stmt>> parse()
    {
        std::vector<std::unique_ptr<Stmt>> program;
        while (!check(TokenType::End))
            program.push_back(statement());
        return program;
    }
};

static std::string astExpr(const Expr &e)
{
    if (e.kind == Expr::Kind::Number)
        return std::to_string(e.number);
    if (e.kind == Expr::Kind::Variable)
        return e.text;
    if (e.kind == Expr::Kind::Unary)
        return "(" + e.text + astExpr(*e.left) + ")";
    return "(" + astExpr(*e.left) + " " + e.text + " " + astExpr(*e.right) + ")";
}
static void printAst(const std::vector<std::unique_ptr<Stmt>> &program, int indent = 0)
{
    std::string pad(static_cast<size_t>(indent), ' ');
    for (const auto &s : program)
    {
        switch (s->kind)
        {
        case Stmt::Kind::Let:
            std::cout << pad << "Let " << s->name << " = " << astExpr(*s->expr) << "\n";
            break;
        case Stmt::Kind::Assign:
            std::cout << pad << "Assign " << s->name << " = " << astExpr(*s->expr) << "\n";
            break;
        case Stmt::Kind::Print:
            std::cout << pad << "Print " << astExpr(*s->expr) << "\n";
            break;
        case Stmt::Kind::If:
            std::cout << pad << "If " << astExpr(*s->expr) << "\n";
            printAst(s->body, indent + 2);
            if (!s->alternative.empty())
            {
                std::cout << pad << "Else\n";
                printAst(s->alternative, indent + 2);
            }
            break;
        case Stmt::Kind::While:
            std::cout << pad << "While " << astExpr(*s->expr) << "\n";
            printAst(s->body, indent + 2);
            break;
        case Stmt::Kind::Block:
            std::cout << pad << "Block\n";
            printAst(s->body, indent + 2);
            break;
        }
    }
}

enum class Op
{
    Const,
    Move,
    Add,
    Sub,
    Mul,
    Div,
    Neg,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    Print,
    Jump,
    JumpFalse
};
struct Instruction
{
    Op op;
    std::string a, b, c;
    long long value = 0;
    int target = -1;
};
static std::string opName(Op op)
{
    switch (op)
    {
    case Op::Const:
        return "CONST";
    case Op::Move:
        return "MOVE";
    case Op::Add:
        return "ADD";
    case Op::Sub:
        return "SUB";
    case Op::Mul:
        return "MUL";
    case Op::Div:
        return "DIV";
    case Op::Neg:
        return "NEG";
    case Op::Eq:
        return "EQ";
    case Op::Ne:
        return "NE";
    case Op::Lt:
        return "LT";
    case Op::Le:
        return "LE";
    case Op::Gt:
        return "GT";
    case Op::Ge:
        return "GE";
    case Op::Print:
        return "PRINT";
    case Op::Jump:
        return "JUMP";
    case Op::JumpFalse:
        return "JUMP_IF_FALSE";
    }
    return "?";
}
class Compiler
{
    std::vector<Instruction> code;
    std::unordered_set<std::string> declared;
    int temp = 0;
    std::string newTemp() { return "$t" + std::to_string(temp++); }
    void emit(Op op, std::string a = "", std::string b = "", std::string c = "", long long value = 0, int target = -1) { code.push_back({op, std::move(a), std::move(b), std::move(c), value, target}); }
    std::string compileExpr(const Expr &e)
    {
        if (e.kind == Expr::Kind::Number)
        {
            std::string t = newTemp();
            emit(Op::Const, t, "", "", e.number);
            return t;
        }
        if (e.kind == Expr::Kind::Variable)
        {
            if (!declared.count(e.text))
                throw LangError("Semantic error: variable '" + e.text + "' is used before declaration");
            return e.text;
        }
        if (e.kind == Expr::Kind::Unary)
        {
            std::string v = compileExpr(*e.left), t = newTemp();
            emit(Op::Neg, t, v);
            return t;
        }
        std::string l = compileExpr(*e.left), r = compileExpr(*e.right), t = newTemp();
        Op op;
        if (e.text == "+")
            op = Op::Add;
        else if (e.text == "-")
            op = Op::Sub;
        else if (e.text == "*")
            op = Op::Mul;
        else if (e.text == "/")
            op = Op::Div;
        else if (e.text == "==")
            op = Op::Eq;
        else if (e.text == "!=")
            op = Op::Ne;
        else if (e.text == "<")
            op = Op::Lt;
        else if (e.text == "<=")
            op = Op::Le;
        else if (e.text == ">")
            op = Op::Gt;
        else
            op = Op::Ge;
        emit(op, t, l, r);
        return t;
    }
    void compileStatements(const std::vector<std::unique_ptr<Stmt>> &statements)
    {
        for (const auto &s : statements)
        {
            switch (s->kind)
            {
            case Stmt::Kind::Let:
            {
                if (declared.count(s->name))
                    throw LangError("Semantic error: variable '" + s->name + "' is already declared");
                std::string v = compileExpr(*s->expr);
                declared.insert(s->name);
                emit(Op::Move, s->name, v);
                break;
            }
            case Stmt::Kind::Assign:
            {
                if (!declared.count(s->name))
                    throw LangError("Semantic error: assignment to undeclared variable '" + s->name + "'");
                std::string v = compileExpr(*s->expr);
                emit(Op::Move, s->name, v);
                break;
            }
            case Stmt::Kind::Print:
            {
                std::string v = compileExpr(*s->expr);
                emit(Op::Print, v);
                break;
            }
            case Stmt::Kind::Block:
                compileStatements(s->body);
                break;
            case Stmt::Kind::If:
            {
                std::string cond = compileExpr(*s->expr);
                size_t jf = code.size();
                emit(Op::JumpFalse, cond);
                compileStatements(s->body);
                if (!s->alternative.empty())
                {
                    size_t j = code.size();
                    emit(Op::Jump);
                    code[jf].target = static_cast<int>(code.size());
                    compileStatements(s->alternative);
                    code[j].target = static_cast<int>(code.size());
                }
                else
                    code[jf].target = static_cast<int>(code.size());
                break;
            }
            case Stmt::Kind::While:
            {
                int start = static_cast<int>(code.size());
                std::string cond = compileExpr(*s->expr);
                size_t jf = code.size();
                emit(Op::JumpFalse, cond);
                compileStatements(s->body);
                emit(Op::Jump, "", "", "", 0, start);
                code[jf].target = static_cast<int>(code.size());
                break;
            }
            }
        }
    }

public:
    std::vector<Instruction> compile(const std::vector<std::unique_ptr<Stmt>> &p)
    {
        compileStatements(p);
        return code;
    }
};

static void optimize(std::vector<Instruction> &code)
{
    // Control-flow-aware data-flow analysis is intentionally out of scope for this first version.
    // Avoid rewriting programs with jumps so constants cannot leak across branches or loop iterations.
    for (const auto &instruction : code)
    {
        if (instruction.op == Op::Jump || instruction.op == Op::JumpFalse)
            return;
    }
    // Straight-line constant folding: fold operations when both operands are known constants.
    std::unordered_map<std::string, long long> constants;
    for (auto &i : code)
    {
        if (i.op == Op::Const)
        {
            constants[i.a] = i.value;
            continue;
        }
        if (i.op == Op::Move)
        {
            if (constants.count(i.b))
                constants[i.a] = constants[i.b];
            else
                constants.erase(i.a);
            continue;
        }
        if (i.op == Op::Neg && constants.count(i.b))
        {
            i.op = Op::Const;
            i.value = -constants[i.b];
            i.b.clear();
            constants[i.a] = i.value;
            continue;
        }
        bool binary = i.op == Op::Add || i.op == Op::Sub || i.op == Op::Mul || i.op == Op::Div || i.op == Op::Eq || i.op == Op::Ne || i.op == Op::Lt || i.op == Op::Le || i.op == Op::Gt || i.op == Op::Ge;
        if (binary)
        {
            auto l = constants.find(i.b), r = constants.find(i.c);
            if (l != constants.end() && r != constants.end() && !(i.op == Op::Div && r->second == 0))
            {
                long long x = l->second, y = r->second, z = 0;
                switch (i.op)
                {
                case Op::Add:
                    z = x + y;
                    break;
                case Op::Sub:
                    z = x - y;
                    break;
                case Op::Mul:
                    z = x * y;
                    break;
                case Op::Div:
                    z = x / y;
                    break;
                case Op::Eq:
                    z = x == y;
                    break;
                case Op::Ne:
                    z = x != y;
                    break;
                case Op::Lt:
                    z = x < y;
                    break;
                case Op::Le:
                    z = x <= y;
                    break;
                case Op::Gt:
                    z = x > y;
                    break;
                case Op::Ge:
                    z = x >= y;
                    break;
                default:
                    break;
                }
                i.op = Op::Const;
                i.value = z;
                i.b.clear();
                i.c.clear();
                constants[i.a] = z;
                continue;
            }
        }
        if (i.op == Op::JumpFalse)
        {
            constants.erase(i.a);
            continue;
        }
        if (i.op == Op::Print || i.op == Op::Jump)
        {
            continue;
        }
        if (!i.a.empty())
            constants.erase(i.a);
    }
}
static void printIR(const std::vector<Instruction> &code)
{
    for (size_t n = 0; n < code.size(); ++n)
    {
        const auto &i = code[n];
        std::cout << n << ": " << opName(i.op);
        if (i.op == Op::Const)
            std::cout << " " << i.a << ", " << i.value;
        else if (i.op == Op::Move)
            std::cout << " " << i.a << ", " << i.b;
        else if (i.op == Op::Neg)
            std::cout << " " << i.a << ", " << i.b;
        else if (i.op == Op::Add || i.op == Op::Sub || i.op == Op::Mul || i.op == Op::Div || i.op == Op::Eq || i.op == Op::Ne || i.op == Op::Lt || i.op == Op::Le || i.op == Op::Gt || i.op == Op::Ge)
            std::cout << " " << i.a << ", " << i.b << ", " << i.c;
        else if (i.op == Op::Print || i.op == Op::JumpFalse)
            std::cout << " " << i.a << (i.op == Op::JumpFalse ? ", -> " + std::to_string(i.target) : "");
        else if (i.op == Op::Jump)
            std::cout << " -> " << i.target;
        std::cout << "\n";
    }
}
class VM
{
    std::unordered_map<std::string, long long> vars;
    long long get(const std::string &n)
    {
        auto it = vars.find(n);
        if (it == vars.end())
            throw LangError("Runtime error: missing value for '" + n + "'");
        return it->second;
    }

public:
    void run(const std::vector<Instruction> &code)
    {
        size_t pc = 0;
        size_t steps = 0;
        while (pc < code.size())
        {
            if (++steps > 10000000)
                throw LangError("Runtime error: instruction limit reached (possible infinite loop)");
            const auto &i = code[pc];
            long long y = 0;
            switch (i.op)
            {
            case Op::Const:
                vars[i.a] = i.value;
                break;
            case Op::Move:
                vars[i.a] = get(i.b);
                break;
            case Op::Neg:
                vars[i.a] = -get(i.b);
                break;
            case Op::Add:
                vars[i.a] = get(i.b) + get(i.c);
                break;
            case Op::Sub:
                vars[i.a] = get(i.b) - get(i.c);
                break;
            case Op::Mul:
                vars[i.a] = get(i.b) * get(i.c);
                break;
            case Op::Div:
                y = get(i.c);
                if (y == 0)
                    throw LangError("Runtime error: division by zero");
                vars[i.a] = get(i.b) / y;
                break;
            case Op::Eq:
                vars[i.a] = get(i.b) == get(i.c);
                break;
            case Op::Ne:
                vars[i.a] = get(i.b) != get(i.c);
                break;
            case Op::Lt:
                vars[i.a] = get(i.b) < get(i.c);
                break;
            case Op::Le:
                vars[i.a] = get(i.b) <= get(i.c);
                break;
            case Op::Gt:
                vars[i.a] = get(i.b) > get(i.c);
                break;
            case Op::Ge:
                vars[i.a] = get(i.b) >= get(i.c);
                break;
            case Op::Print:
                std::cout << get(i.a) << "\n";
                break;
            case Op::Jump:
                pc = static_cast<size_t>(i.target);
                continue;
            case Op::JumpFalse:
                if (get(i.a) == 0)
                {
                    pc = static_cast<size_t>(i.target);
                    continue;
                }
                break;
            }
            ++pc;
        }
    }
};

static std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    if (!f)
        throw LangError("Could not open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
static void usage() { std::cout << "MiniLang Compiler (educational C++17 project)\n\nUsage: minilang [--tokens|--ast|--ir|--optimize] <file.mini>\n       minilang <file.mini>   Compile and execute\n\nOptions:\n  --tokens     Print lexer tokens\n  --ast        Print the abstract syntax tree\n  --ir         Print generated intermediate representation\n  --optimize   Optimize IR, print it, then execute\n  --help       Show this help\n"; }
int main(int argc, char **argv)
{
    try
    {
        if (argc < 2)
        {
            usage();
            return 0;
        }
        std::string mode = "run", path;
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h")
        {
            usage();
            return 0;
        }
        if (arg.rfind("--", 0) == 0)
        {
            mode = arg.substr(2);
            if (argc < 3)
            {
                usage();
                return 2;
            }
            path = argv[2];
        }
        else
            path = arg;
        std::string source = readFile(path);
        Lexer lexer(source);
        auto tokens = lexer.scan();
        if (mode == "tokens")
        {
            for (const auto &t : tokens)
                std::cout << t.line << ":" << t.column << "  " << tokenName(t.type) << (t.lexeme.empty() ? "" : "  " + t.lexeme) << "\n";
            return 0;
        }
        Parser parser(tokens);
        auto ast = parser.parse();
        if (mode == "ast")
        {
            printAst(ast);
            return 0;
        }
        Compiler compiler;
        auto code = compiler.compile(ast);
        if (mode == "ir")
        {
            printIR(code);
            return 0;
        }
        if (mode == "optimize")
        {
            optimize(code);
            std::cout << "Optimized IR:\n";
            printIR(code);
            std::cout << "\nProgram output:\n";
            VM vm;
            vm.run(code);
            return 0;
        }
        if (mode != "run")
            throw LangError("Unknown option: --" + mode);
        VM vm;
        vm.run(code);
        return 0;
    }
    catch (const LangError &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
}
