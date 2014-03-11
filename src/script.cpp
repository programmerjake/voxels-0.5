#include "script.h"
#include "util.h"
#include <cwctype>
#include <sstream>

using namespace std;

namespace
{
struct Parser
{
    shared_ptr<Script> script;
    wstring code;
    Parser(wstring code)
        : script(make_shared<Script>()), code(code)
    {
        init();
    }
    wint_t curChar = L' ', pushedChar;
    bool hasPushedChar = false;
    size_t nextStringIndex = 0;
    int curCol = 0, curLine = 1;
    void errorFn(wstring msg)
    {
        wostringstream os;
        os << L"parse error : Line " << curLine << L" Column " << curCol << L" : " << msg;
        throw runtime_error(wcsrtombs(os.str()));
    }
    void getChar()
    {
        if(hasPushedChar)
        {
            curChar = pushedChar;
            hasPushedChar = false;
            return;
        }
        if(curChar == WEOF)
            return;
        if(curChar == L'\n')
        {
            curCol = 1;
            curLine++;
        }
        else
            curCol++;
        if(nextStringIndex >= code.size())
            curChar = WEOF;
        else
            curChar = code[nextStringIndex++];
    }
    void putBackChar(wint_t ch)
    {
        pushedChar = curChar;
        curChar = ch;
        hasPushedChar = true;
    }
    enum class TokenType
    {
        EndOfFile,
        IntegerLit,
        FloatLit,
        StringLit,
        BooleanLit,
        Id,
        If,
        Else,
        And,
        Or,
        Xor,
        Abs,
        Cast,
        Boolean,
        Object,
        String,
        Integer,
        Float,
        Matrix,
        Vector,
        List,
        DotProd,
        CrossProd,
        Sin,
        Cos,
        Tan,
        ASin,
        ACos,
        ATan,
        Exp,
        Log,
        Sqrt,
        ATan2,
        MakeRotate,
        MakeRotateX,
        MakeRotateY,
        MakeRotateZ,
        MakeScale,
        MakeTranslate,
        LBracket,
        RBracket,
        LBrace,
        RBrace,
        Comma,
        Pipe,
        LParen,
        RParen,
        LAngle,
        RAngle,
        Assign,
        Equal,
        NotEqual,
        LessEqual,
        GreaterEqual,
        Plus,
        Minus,
        Star,
        FSlash,
        Percent,
        Colon,
        Semicolon,
        QMark,
        Period,
        Pow,
        Tilde,
        Pi,
    };
    TokenType tokenType = Comma;
    wstring tokenText = L"";
    int tokLine, tokCol;
    unordered_map<wstring, TokenType> wordTokens;

    void init()
    {
        wordTokens[L"false"] = TokenType::BooleanLit;
        wordTokens[L"true"] = TokenType::BooleanLit;
        wordTokens[L"if"] = TokenType::If;
        wordTokens[L"else"] = TokenType::Else;
        wordTokens[L"and"] = TokenType::And;
        wordTokens[L"or"] = TokenType::Or;
        wordTokens[L"xor"] = TokenType::Xor;
        wordTokens[L"abs"] = TokenType::Abs;
        wordTokens[L"cast"] = TokenType::Cast;
        wordTokens[L"boolean"] = TokenType::Boolean;
        wordTokens[L"object"] = TokenType::Object;
        wordTokens[L"string"] = TokenType::String;
        wordTokens[L"integer"] = TokenType::Integer;
        wordTokens[L"float"] = TokenType::Float;
        wordTokens[L"matrix"] = TokenType::Matrix;
        wordTokens[L"vector"] = TokenType::Vector;
        wordTokens[L"list"] = TokenType::List;
        wordTokens[L"dot"] = TokenType::DotProd;
        wordTokens[L"cross"] = TokenType::CrossProd;
        wordTokens[L"sin"] = TokenType::Sin;
        wordTokens[L"cos"] = TokenType::Cos;
        wordTokens[L"tan"] = TokenType::Tan;
        wordTokens[L"asin"] = TokenType::ASin;
        wordTokens[L"acos"] = TokenType::ACos;
        wordTokens[L"atan"] = TokenType::ATan;
        wordTokens[L"exp"] = TokenType::Exp;
        wordTokens[L"log"] = TokenType::Log;
        wordTokens[L"sqrt"] = TokenType::Sqrt;
        wordTokens[L"atan2"] = TokenType::ATan2;
        wordTokens[L"make_rotate"] = TokenType::MakeRotate;
        wordTokens[L"make_rotatex"] = TokenType::MakeRotateX;
        wordTokens[L"make_rotatey"] = TokenType::MakeRotateY;
        wordTokens[L"make_rotatez"] = TokenType::MakeRotateZ;
        wordTokens[L"make_scale"] = TokenType::MakeScale;
        wordTokens[L"make_translate"] = TokenType::MakeTranslate;
        wordTokens[L"pi"] = TokenType::Pi;
    }

    void skipWhitespace()
    {
        while(iswspace(curChar) || curChar == L'/')
        {
            if(curChar == L'/')
            {
                getChar();
                if(curChar == L'/')
                {
                    while(curChar != WEOF && curChar != '\n')
                        getChar();
                }
                else if(curChar == L'*')
                {
                    getChar();
                    int startLine = curLine, startCol = curCol;
                    while(curChar != WEOF)
                    {
                        if(curChar == L'*')
                        {
                            while(curChar == L'*')
                                getChar();
                            if(curChar == L'/')
                                break;
                        }
                        else
                            getChar();
                    }
                    if(curChar == WEOF)
                        errorFn((wostringstream() << L"expected : */ : opening /* on line " << startLine << L" col " << startCol).str());
                    getChar();
                }
                else
                {
                    putBackChar(L'/');
                    return;
                }
            }
            else
                getChar();
        }
    }

    void getToken()
    {
        tokenType = TokenType::EndOfFile;
        tokenText = L"";
        skipWhitespace();
        if(curChar == WEOF)
        {
            return;
        }
        if(curChar == L'.')
        {
            tokenType = TokenType::Period;
            tokenText = L".";
            getChar();
            if(!iswdigit(curChar))
            {
                return;
            }
            while(iswdigit(curChar))
            {
                tokenText += curChar;
                getChar();
            }
            if(curChar == L'e')
            {
                tokenText += curChar;
                getChar();
                if(curChar == L'-' || curChar == L'+')
                {
                    tokenText += curChar;
                    getChar();
                }
                if(!iswdigit(curChar))
                    errorFn("floating point exponent missing digits");
                while(iswdigit(curChar))
                {
                    tokenText += curChar;
                    getChar();
                }
            }
            if(curChar == L'f')
            {
                tokenText += curChar;
                getChar();
            }
            tokenType = TokenType::FloatLit;
            return;
        }
        if(iswdigit(curChar))
        {
            tokenType = TokenType::IntegerLit;
            while(iswdigit(curChar))
            {
                tokenText += curChar;
                getChar();
            }
            if(curChar == L'.')
            {
                tokenType = TokenType::FloatLit;
                tokenText += curChar;
                getChar();
                while(iswdigit(curChar))
                {
                    tokenText += curChar;
                    getChar();
                }
            }
            if(curChar == L'e')
            {
                tokenType = TokenType::FloatLit;
                tokenText += curChar;
                getChar();
                if(curChar == L'-' || curChar == L'+')
                {
                    tokenText += curChar;
                    getChar();
                }
                if(!iswdigit(curChar))
                    errorFn("floating point exponent missing digits");
                while(iswdigit(curChar))
                {
                    tokenText += curChar;
                    getChar();
                }
            }
            if(curChar == L'f')
            {
                tokenType = TokenType::FloatLit;
                tokenText += curChar;
                getChar();
            }
            return;
        }
        if(iswalpha(curChar) || curChar == L'_')
        {
            tokenType = TokenType::Id;
            while(iswalnum(curChar) || curChar == L'_')
            {
                tokenText += curChar;
                getChar();
            }
            auto iter = wordTokens.find(tokenText);
            if(iter != wordTokens.end())
                tokenType = get<1>(*iter);
            return;
        }
        if(curChar == L'\"')
        {
            getChar();
            tokenType = TokenType::StringLit;
            while(curChar != WEOF)
            {
                if(curChar == L'\"')
                {
                    getChar();
                    skipWhitespace();
                    if(curChar == L'\"')
                        getChar();
                    else
                        return;
                }
                else if(curChar == L'\\')
                {
                    getChar();
                    switch(curChar)
                    {
                    case L'\\':
                        tokenText += L'\\';
                        getChar();
                        break;
                    case L'\"':
                        tokenText += L'\"';
                        getChar();
                        break;
                    case L'\'':
                        tokenText += L'\'';
                        getChar();
                        break;
                    case L'a':
                        tokenText += L'\a';
                        getChar();
                        break;
                    case L'b':
                        tokenText += L'\b';
                        getChar();
                        break;
                    case L'e':
                        tokenText += L'\x1B';
                        getChar();
                        break;
                    case L'f':
                        tokenText += L'\f';
                        getChar();
                        break;
                    case L'n':
                        tokenText += L'\n';
                        getChar();
                        break;
                    case L'r':
                        tokenText += L'\r';
                        getChar();
                        break;
                    case L't':
                        tokenText += L'\t';
                        getChar();
                        break;
                    case L'v':
                        tokenText += L'\v';
                        getChar();
                        break;
                    case L'0':
                        tokenText += L'\0';
                        getChar();
                        break;
                    case L'x':
                    {
                        getChar();
                        wchar_t v = 0;
                        for(int i = 0; i < 2; i++)
                        {
                            if(!iswxdigit(curChar))
                                errorFn(L"expected : hex digit");
                            v <<= 4;
                            if(iswupper(curChar))
                                v += 0xA + curChar - L'A';
                            else if(iswlower(curChar))
                                v += 0xA + curChar - L'a';
                            else
                                v += curChar - L'0';
                            getChar();
                        }
                        tokenText += v;
                        getChar();
                        break;
                    }
                    case L'u':
                    {
                        getChar();
                        wchar_t v = 0;
                        for(int i = 0; i < 4; i++)
                        {
                            if(!iswxdigit(curChar))
                                errorFn(L"expected : hex digit");
                            v <<= 4;
                            if(iswupper(curChar))
                                v += 0xA + curChar - L'A';
                            else if(iswlower(curChar))
                                v += 0xA + curChar - L'a';
                            else
                                v += curChar - L'0';
                            getChar();
                        }
                        if(v >= 0xD800 && v <= 0xDFFF)
                            errorFn(L"can't use unicode code points in the surrogate ranges");
                        if(v >= 0x110000)
                            errorFn(L"code point out of range");
                        tokenText += v;
                        getChar();
                        break;
                    }
                    case L'U':
                    {
                        getChar();
                        uint32_t v = 0;
                        for(int i = 0; i < 8; i++)
                        {
                            if(!iswxdigit(curChar))
                                errorFn(L"expected : hex digit");
                            v <<= 4;
                            if(iswupper(curChar))
                                v += 0xA + curChar - L'A';
                            else if(iswlower(curChar))
                                v += 0xA + curChar - L'a';
                            else
                                v += curChar - L'0';
                            getChar();
                        }
                        if(v >= 0xD800 && v <= 0xDFFF)
                            errorFn(L"can't use unicode code points in the surrogate ranges");
                        if(v >= 0x110000)
                            errorFn(L"code point out of range");
                        tokenText += (wchar_t)v;
                        getChar();
                        break;
                    }
                    default:
                        errorFn(L"illegal escape sequence");
                    }
                }
                else
                {
                    tokenText += curChar;
                    getChar();
                }
            }
            errorFn(L"missing : closing \"");
        }
        switch(curChar)
        {
        case L'[':
            tokenType = TokenType::LBracket;
            tokenText += curChar;
            getChar();
            break;
        case L']':
            tokenType = TokenType::RBracket;
            tokenText += curChar;
            getChar();
            break;
        case L'{':
            tokenType = TokenType::LBrace;
            tokenText += curChar;
            getChar();
            break;
        case L'}':
            tokenType = TokenType::RBrace;
            tokenText += curChar;
            getChar();
            break;
        case L',':
            tokenType = TokenType::Comma;
            tokenText += curChar;
            getChar();
            break;
        case L'|':
            tokenType = TokenType::Pipe;
            tokenText += curChar;
            getChar();
            break;
        case L'(':
            tokenType = TokenType::LParen;
            tokenText += curChar;
            getChar();
            break;
        case L')':
            tokenType = TokenType::RParen;
            tokenText += curChar;
            getChar();
            break;
        case L'<':
            tokenType = TokenType::LAngle;
            tokenText += curChar;
            getChar();
            if(curChar == L'=')
            {
                tokenType = TokenType::LessEqual;
                tokenText += curChar;
                getChar();
            }
            break;
        case L'>':
            tokenType = TokenType::RAngle;
            tokenText += curChar;
            getChar();
            if(curChar == L'=')
            {
                tokenType = TokenType::GreaterEqual;
                tokenText += curChar;
                getChar();
            }
            break;
        case L'=':
            tokenType = TokenType::Assign;
            tokenText += curChar;
            getChar();
            if(curChar == L'=')
            {
                tokenType = TokenType::Equal;
                tokenText += curChar;
                getChar();
            }
            break;
        case L'+':
            tokenType = TokenType::Plus;
            tokenText += curChar;
            getChar();
            break;
        case L'-':
            tokenType = TokenType::Minus;
            tokenText += curChar;
            getChar();
            break;
        case L'*':
            tokenType = TokenType::Star;
            tokenText += curChar;
            getChar();
            if(curChar == L'*')
            {
                tokenType = TokenType::Pow;
                tokenText += curChar;
                getChar();
            }
            break;
        case L'/':
            tokenType = TokenType::FSlash;
            tokenText += curChar;
            getChar();
            break;
        case L'%':
            tokenType = TokenType::Percent;
            tokenText += curChar;
            getChar();
            break;
        case L':':
            tokenType = TokenType::Colon;
            tokenText += curChar;
            getChar();
            break;
        case L';':
            tokenType = TokenType::Semicolon;
            tokenText += curChar;
            getChar();
            break;
        case L'?':
            tokenType = TokenType::QMark;
            tokenText += curChar;
            getChar();
            break;
        case L'.':
            tokenType = TokenType::Period;
            tokenText += curChar;
            getChar();
            break;
        case L'~':
            tokenType = TokenType::Tilde;
            tokenText += curChar;
            getChar();
            break;
        case L'!':
            tokenText += curChar;
            getChar();
            if(curChar == L'=')
            {
                tokenType = TokenType::NotEqual;
                tokenText += curChar;
                getChar();
            }
            else
                errorFn("missing = in !=");
            break;
        default:
            errorFn("illegal character");
        }
    }

    uint32_t insertNode(shared_ptr<Script::Node> node)
    {
        uint32_t retval = script->nodes.size();
        script->nodes.push_back(node);
        return retval;
    }

    uint32_t parseExpression(TokenType ignoreType);

    uint32_t parseTopLevel()
    {
        switch(tokenType)
        {
        case TokenType::LAngle:
        {
            getToken();
            uint32_t x = parseExpression(TokenType::Comma);
            if(tokenType != TokenType::Comma)
                errorFn(L"expected : ,");
            getToken();
            uint32_t y = parseExpression(TokenType::Comma);
            if(tokenType != TokenType::Comma)
                errorFn(L"expected : ,");
            getToken();
            uint32_t z = parseExpression(TokenType::RAngle);
            if(tokenType != TokenType::RAngle)
                errorFn(L"expected : >");
            getToken();
            shared_ptr<Script::NodeListLiteral> listNode = make_shared<Script::NodeListLiteral>();
            listNode->nodes.push_back(x);
            listNode->nodes.push_back(y);
            listNode->nodes.push_back(z);
            uint32_t list = insertNode(static_pointer_cast<Script::Node>(listNode));
            shared_ptr<Script::NodeCastToVector> vectorNode = make_shared<Script::NodeCastToVector>();
            vectorNode->args[0] = list;
            return insertNode(static_pointer_cast<Script::Node>(vectorNode));
        }
        case TokenType::LBracket:
        {
            getToken();
            shared_ptr<Script::NodeListLiteral> listNode = make_shared<Script::NodeListLiteral>();
            if(tokenType == TokenType::RBracket)
            {
                getToken();
                return insertNode(static_pointer_cast<Script::Node>(listNode));
            }
            while(true)
            {
                uint32_t e = parseExpression(TokenType::Comma);
                listNode->nodes.push_back(e);
                if(tokenType == TokenType::RBracket)
                    break;
                if(tokenType != TokenType::Comma)
                    errorFn(L"expected : , or ]");
                getToken();
            }
            getToken();
            return insertNode(static_pointer_cast<Script::Node>(listNode));
        }
        case TokenType::LParen:
        {
            getToken();
            uint32_t retval = parseExpression(TokenType::RParen);
            if(tokenType != TokenType::RParen)
                errorFn(L"expected : )");
            getToken();
            return retval;
        }
        case TokenType::Id:
        {
            uint32_t globals = insertNode(shared_ptr<Script::Node>(new Script::NodeLoadGlobals));
            uint32_t name = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataString(tokenText)))));
            getToken();
            shared_ptr<Script::NodeReadIndex> readIndexNode = make_shared<Script::NodeReadIndex>();
            readIndexNode->args[0] = globals;
            readIndexNode->args[1] = name;
            return insertNode(static_pointer_cast<Script::Node>(readIndexNode));
        }
        case TokenType::LBrace:
        {
            getToken();
            shared_ptr<Script::NodeListLiteral> listNode = make_shared<Script::NodeListLiteral>();
            while(tokenType == TokenType::Semicolon)
                getToken();
            if(tokenType == TokenType::RBrace)
            {
                getToken();
                return insertNode(static_pointer_cast<Script::Node>(listNode));
            }
            while(true)
            {
                uint32_t e = parseExpression(TokenType::Comma);
                listNode->nodes.push_back(e);
                if(tokenType == TokenType::RBrace)
                    break;
                if(tokenType != TokenType::Semicolon)
                    errorFn(L"expected : ; or }");
                while(tokenType == TokenType::Semicolon)
                    getToken();
            }
            getToken();
            return insertNode(static_pointer_cast<Script::Node>(listNode));
        }
        case TokenType::StringLit:
        {
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataString(tokenText)))));
            getToken();
            return retval;
        }
        case TokenType::IntegerLit:
        {
            int32_t v;
            wistringstream(tokenText) >> v;
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataInteger(v)))));
            getToken();
            return retval;
        }
        case TokenType::FloatLit:
        {
            float v;
            wistringstream(tokenText) >> v;
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataFloat(v)))));
            getToken();
            return retval;
        }
        case TokenType::BooleanLit:
        {
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataBoolean(tokenText[0] == L't')))));
            getToken();
            return retval;
        }
        case TokenType::Pi:
        {
            getToken();
            return insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataFloat(M_PI)))));
        }
        default:
            errorFn(L"unexpected token");
        }
    }

    shared_ptr<Script> run()
    {
        getChar();
        getToken();
        uint32_t node = parseExpression(TokenType::EndOfFile);
        assert(node == script->nodes.size() - 1);
        if(tokenType != TokenType::EndOfFile)
            errorFn(L"unexpected token");
        return script;
    }
};
}

shared_ptr<Script> Script::parse(wstring code)
{
    Parser parser(code);
    return parser.run();
}
