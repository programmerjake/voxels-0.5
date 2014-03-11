#include "script.h"
#include "util.h"
#include <cwctype>
#include <sstream>
#include <cstdlib>
#include <iostream>

using namespace std;

namespace
{
struct Parser
{
    shared_ptr<Script> script;
    wstring code;
    Parser(wstring code)
        : script(make_shared<Script>()), code(L"{ " + code + L" }")
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
        os << L"parse error : Line " << tokLine << L" Column " << tokCol << L" : " << msg;
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
        {
            return;
        }

        if(curChar == L'\n')
        {
            curCol = 1;
            curLine++;
        }
        else
        {
            curCol++;
        }

        if(nextStringIndex >= code.size())
        {
            curChar = WEOF;
        }
        else
        {
            curChar = code[nextStringIndex++];
        }
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
        Not,
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
        New,
    };
    TokenType tokenType = TokenType::Comma;
    wstring tokenText = L"";
    bool hasPutbackToken = false;
    TokenType lastToken;
    wstring lastTokenText;
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
        wordTokens[L"not"] = TokenType::Not;
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
        wordTokens[L"new"] = TokenType::New;
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
                    {
                        getChar();
                    }
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
                            {
                                getChar();
                            }

                            if(curChar == L'/')
                            {
                                break;
                            }
                        }
                        else
                        {
                            getChar();
                        }
                    }

                    if(curChar == WEOF)
                    {
                        wostringstream os;
                        os << L"expected : */ : opening /* on line " << startLine << L" col " << startCol;
                        tokLine = curLine;
                        tokCol = curCol;
                        errorFn(os.str());
                    }

                    getChar();
                }
                else
                {
                    putBackChar(L'/');
                    return;
                }
            }
            else
            {
                getChar();
            }
        }
    }

    void getToken()
    {
        if(hasPutbackToken)
        {
            hasPutbackToken = false;
            tokenType = lastToken;
            tokenText = lastTokenText;
            return;
        }

        tokenType = TokenType::EndOfFile;
        tokenText = L"";
        skipWhitespace();
        tokLine = curLine;
        tokCol = curCol;

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
                {
                    tokLine = curLine;
                    tokCol = curCol;
                    errorFn(L"floating point exponent missing digits");
                }

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
                {
                    tokLine = curLine;
                    tokCol = curCol;
                    errorFn(L"floating point exponent missing digits");
                }

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
            {
                tokenType = get<1>(*iter);
            }

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
                    {
                        getChar();
                    }
                    else
                    {
                        return;
                    }
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
                            {
                                tokLine = curLine;
                                tokCol = curCol;
                                errorFn(L"expected : hex digit");
                            }

                            v <<= 4;

                            if(iswupper(curChar))
                            {
                                v += 0xA + curChar - L'A';
                            }
                            else if(iswlower(curChar))
                            {
                                v += 0xA + curChar - L'a';
                            }
                            else
                            {
                                v += curChar - L'0';
                            }

                            getChar();
                        }

                        tokenText += v;
                        break;
                    }

                    case L'u':
                    {
                        getChar();
                        wchar_t v = 0;

                        for(int i = 0; i < 4; i++)
                        {
                            if(!iswxdigit(curChar))
                            {
                                tokLine = curLine;
                                tokCol = curCol;
                                errorFn(L"expected : hex digit");
                            }

                            v <<= 4;

                            if(iswupper(curChar))
                            {
                                v += 0xA + curChar - L'A';
                            }
                            else if(iswlower(curChar))
                            {
                                v += 0xA + curChar - L'a';
                            }
                            else
                            {
                                v += curChar - L'0';
                            }

                            getChar();
                        }

                        if(v >= 0xD800 && v <= 0xDFFF)
                        {
                            tokLine = curLine;
                            tokCol = curCol;
                            errorFn(L"can't use unicode code points in the surrogate ranges");
                        }

                        if(v >= 0x110000)
                        {
                            tokLine = curLine;
                            tokCol = curCol;
                            errorFn(L"code point out of range");
                        }

                        tokenText += v;
                        break;
                    }

                    case L'U':
                    {
                        getChar();
                        uint32_t v = 0;

                        for(int i = 0; i < 8; i++)
                        {
                            if(!iswxdigit(curChar))
                            {
                                tokLine = curLine;
                                tokCol = curCol;
                                errorFn(L"expected : hex digit");
                            }

                            v <<= 4;

                            if(iswupper(curChar))
                            {
                                v += 0xA + curChar - L'A';
                            }
                            else if(iswlower(curChar))
                            {
                                v += 0xA + curChar - L'a';
                            }
                            else
                            {
                                v += curChar - L'0';
                            }

                            getChar();
                        }

                        if(v >= 0xD800 && v <= 0xDFFF)
                        {
                            tokLine = curLine;
                            tokCol = curCol;
                            errorFn(L"can't use unicode code points in the surrogate ranges");
                        }

                        if(v >= 0x110000)
                        {
                            tokLine = curLine;
                            tokCol = curCol;
                            errorFn(L"code point out of range");
                        }

                        tokenText += (wchar_t)v;
                        break;
                    }

                    default:
                        tokLine = curLine;
                        tokCol = curCol;
                        errorFn(L"illegal escape sequence");
                    }
                }
                else
                {
                    tokenText += curChar;
                    getChar();
                }
            }

            tokLine = curLine;
            tokCol = curCol;
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
            {
                tokLine = curLine;
                tokCol = curCol;
                errorFn(L"missing = in !=");
            }

            break;

        default:
            tokLine = curLine;
            tokCol = curCol;
            errorFn(L"illegal character");
        }
    }

    void putBackToken(TokenType tt, wstring ts)
    {
        hasPutbackToken = true;
        lastToken = tokenType;
        lastTokenText = tokenText;
        tokenType = tt;
        tokenText = ts;
    }

    uint32_t insertNode(shared_ptr<Script::Node> node)
    {
        uint32_t retval = script->nodes.size();
        script->nodes.push_back(node);
        return retval;
    }

    uint32_t parseTopLevel()
    {
        switch(tokenType)
        {
        case TokenType::LAngle:
        {
            getToken();
            uint32_t x = parseExpression(TokenType::Comma);

            if(tokenType != TokenType::Comma)
            {
                errorFn(L"expected : ,");
            }

            getToken();
            uint32_t y = parseExpression(TokenType::Comma);

            if(tokenType != TokenType::Comma)
            {
                errorFn(L"expected : ,");
            }

            getToken();
            uint32_t z = parseExpression(TokenType::RAngle);

            if(tokenType != TokenType::RAngle)
            {
                errorFn(L"expected : >");
            }

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
                {
                    break;
                }

                if(tokenType != TokenType::Comma)
                {
                    errorFn(L"expected : , or ]");
                }

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
            {
                errorFn(L"expected : )");
            }

            getToken();
            return retval;
        }

        case TokenType::Id:
        {
            uint32_t globals = insertNode(shared_ptr<Script::Node>(new Script::NodeLoadGlobals));
            uint32_t name = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>
                                       (new Script::DataString(tokenText)))));
            getToken();
            shared_ptr<Script::NodeReadIndex> readIndexNode = make_shared<Script::NodeReadIndex>();
            readIndexNode->args[0] = globals;
            readIndexNode->args[1] = name;
            return insertNode(static_pointer_cast<Script::Node>(readIndexNode));
        }

        case TokenType::LBrace:
        {
            getToken();
            shared_ptr<Script::NodeBlock> listNode = make_shared<Script::NodeBlock>();

            while(tokenType == TokenType::Semicolon)
            {
                getToken();
            }

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
                {
                    break;
                }

                if(tokenType != TokenType::Semicolon)
                {
                    errorFn(L"expected : ; or }");
                }

                while(tokenType == TokenType::Semicolon)
                {
                    getToken();
                }
            }

            getToken();
            return insertNode(static_pointer_cast<Script::Node>(listNode));
        }

        case TokenType::StringLit:
        {
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(
                                             shared_ptr<Script::Data>(new Script::DataString(tokenText)))));
            getToken();
            return retval;
        }

        case TokenType::IntegerLit:
        {
            int32_t v;
            wistringstream(tokenText) >> v;
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(
                                             shared_ptr<Script::Data>(new Script::DataInteger(v)))));
            getToken();
            return retval;
        }

        case TokenType::FloatLit:
        {
            float v;
            wistringstream(tokenText) >> v;
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(
                                             shared_ptr<Script::Data>(new Script::DataFloat(v)))));
            getToken();
            return retval;
        }

        case TokenType::BooleanLit:
        {
            uint32_t retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(
                                             shared_ptr<Script::Data>(new Script::DataBoolean(tokenText[0] == L't')))));
            getToken();
            return retval;
        }

        case TokenType::Pi:
        {
            getToken();
            return insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>
                              (new Script::DataFloat(M_PI)))));
        }

        case TokenType::Sin:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeSin> node = make_shared<Script::NodeSin>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::Cos:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeCos> node = make_shared<Script::NodeCos>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::Tan:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeTan> node = make_shared<Script::NodeTan>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::ASin:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeASin> node = make_shared<Script::NodeASin>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::ACos:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeACos> node = make_shared<Script::NodeACos>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::ATan:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeATan> node = make_shared<Script::NodeATan>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::Exp:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeExp> node = make_shared<Script::NodeExp>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::Log:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeLog> node = make_shared<Script::NodeLog>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::Sqrt:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeSqrt> node = make_shared<Script::NodeSqrt>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::MakeRotateX:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeMakeRotateX> node = make_shared<Script::NodeMakeRotateX>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::MakeRotateY:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeMakeRotateY> node = make_shared<Script::NodeMakeRotateY>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::MakeRotateZ:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeMakeRotateZ> node = make_shared<Script::NodeMakeRotateZ>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::MakeScale:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeMakeScale> node = make_shared<Script::NodeMakeScale>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::MakeTranslate:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeMakeTranslate> node = make_shared<Script::NodeMakeTranslate>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::Abs:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeAbs> node = make_shared<Script::NodeAbs>();
            node->args[0] = expr;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::ATan2:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr1 = parseExpression(TokenType::Comma);

            if(tokenType != TokenType::Comma)
            {
                errorFn(L"expected : ,");
            }

            getToken();
            uint32_t expr2 = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeATan2> node = make_shared<Script::NodeATan2>();
            node->args[0] = expr1;
            node->args[1] = expr2;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        case TokenType::MakeRotate:
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            uint32_t expr1 = parseExpression(TokenType::Comma);

            if(tokenType != TokenType::Comma)
            {
                errorFn(L"expected : ,");
            }

            getToken();
            uint32_t expr2 = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            shared_ptr<Script::NodeMakeRotate> node = make_shared<Script::NodeMakeRotate>();
            node->args[0] = expr1;
            node->args[1] = expr2;
            return insertNode(static_pointer_cast<Script::Node>(node));
        }
        case TokenType::New:
        {
            getToken();
            uint32_t retval;
            switch(tokenType)
            {
            case TokenType::Boolean:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataBoolean(false)))));
                break;

            case TokenType::Float:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataFloat(0)))));
                break;

            case TokenType::Integer:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataInteger(0)))));
                break;

            case TokenType::List:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeListLiteral));
                break;

            case TokenType::Matrix:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataMatrix(Matrix::identity())))));
                break;

            case TokenType::Object:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeNewObject));
                break;

            case TokenType::String:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataString(L"")))));
                break;

            case TokenType::Vector:
                retval = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>(new Script::DataVector(VectorF(0))))));
                break;

            default:
                errorFn(L"expected : type");
            }
            getToken();
            return retval;
        }
        default:
            errorFn(L"unexpected token");
        }

        assert(false);
        return 0;
    }

    uint32_t parseElementAccess()
    {
        uint32_t retval = parseTopLevel();

        while(tokenType == TokenType::LBracket || tokenType == TokenType::Period)
        {
            if(tokenType == TokenType::LBracket)
            {
                getToken();
                uint32_t arg = parseExpression(TokenType::RBracket);

                if(tokenType != TokenType::RBracket)
                {
                    errorFn(L"expected : ]");
                }

                getToken();
                shared_ptr<Script::NodeReadIndex> readIndexNode = make_shared<Script::NodeReadIndex>();
                readIndexNode->args[0] = retval;
                readIndexNode->args[1] = arg;
                retval = insertNode(static_pointer_cast<Script::Node>(readIndexNode));
            }
            else if(tokenType == TokenType::Period)
            {
                getToken();

                if(tokenType != TokenType::Id)
                {
                    errorFn(L"expected : name");
                }

                uint32_t name = insertNode(shared_ptr<Script::Node>(new Script::NodeConst(shared_ptr<Script::Data>
                                           (new Script::DataString(tokenText)))));
                getToken();
                shared_ptr<Script::NodeReadIndex> readIndexNode = make_shared<Script::NodeReadIndex>();
                readIndexNode->args[0] = retval;
                readIndexNode->args[1] = name;
                retval = insertNode(static_pointer_cast<Script::Node>(readIndexNode));
            }
            else
            {
                assert(false);
            }
        }

        return retval;
    }

    uint32_t parseCasts()
    {
        if(tokenType == TokenType::Cast)
        {
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            shared_ptr<Script::NodeCast> cast;

            switch(tokenType)
            {
            case TokenType::Boolean:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToBoolean>());
                break;

            case TokenType::Float:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToFloat>());
                break;

            case TokenType::Integer:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToInteger>());
                break;

            case TokenType::List:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToList>());
                break;

            case TokenType::Matrix:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToMatrix>());
                break;

            case TokenType::Object:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToObject>());
                break;

            case TokenType::String:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToString>());
                break;

            case TokenType::Vector:
                cast = static_pointer_cast<Script::NodeCast>(make_shared<Script::NodeCastToVector>());
                break;

            default:
                errorFn(L"expected : type");
            }

            getToken();

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            uint32_t v = parseNeg(TokenType::Pow);
            cast->args[0] = v;
            return insertNode(static_pointer_cast<Script::Node>(cast));
        }

        return parseElementAccess();
    }

    uint32_t parsePow(TokenType ignoreType)
    {
        uint32_t retval = parseElementAccess();

        if(tokenType == TokenType::Pow && ignoreType != tokenType)
        {
            getToken();
            shared_ptr<Script::NodePow> node = make_shared<Script::NodePow>();
            uint32_t arg = parseNeg(ignoreType);
            node->args[0] = retval;
            node->args[1] = arg;
            retval = insertNode(static_pointer_cast<Script::Node>(node));
        }

        return retval;
    }

    uint32_t parseNeg(TokenType ignoreType)
    {
        if(tokenType == TokenType::Minus)
        {
            getToken();
            shared_ptr<Script::NodeNeg> node = make_shared<Script::NodeNeg>();
            node->args[0] = parsePow(ignoreType);
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        return parsePow(ignoreType);
    }

    uint32_t parseFactor(TokenType ignoreType)
    {
        uint32_t retval = parseNeg(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::DotProd)
            {
                auto node = make_shared<Script::NodeDot>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseNeg(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::Star)
            {
                auto node = make_shared<Script::NodeMul>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseNeg(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::Percent)
            {
                auto node = make_shared<Script::NodeMod>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseNeg(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::FSlash)
            {
                auto node = make_shared<Script::NodeDiv>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseNeg(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseCross(TokenType ignoreType)
    {
        uint32_t retval = parseFactor(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::CrossProd)
            {
                auto node = make_shared<Script::NodeCross>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseFactor(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseTerm(TokenType ignoreType)
    {
        uint32_t retval = parseCross(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::Plus)
            {
                auto node = make_shared<Script::NodeAdd>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseCross(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::Minus)
            {
                auto node = make_shared<Script::NodeSub>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseCross(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseConcat(TokenType ignoreType)
    {
        uint32_t retval = parseTerm(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::Tilde)
            {
                auto node = make_shared<Script::NodeConcat>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseTerm(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseCompare(TokenType ignoreType)
    {
        uint32_t retval = parseConcat(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::Equal)
            {
                auto node = make_shared<Script::NodeEqual>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseConcat(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::NotEqual)
            {
                auto node = make_shared<Script::NodeNotEqual>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseConcat(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::LAngle)
            {
                auto node = make_shared<Script::NodeLessThan>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseConcat(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::RAngle)
            {
                auto node = make_shared<Script::NodeGreaterThan>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseConcat(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::GreaterEqual)
            {
                auto node = make_shared<Script::NodeGreaterEqual>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseConcat(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else if(tokenType == TokenType::LessEqual)
            {
                auto node = make_shared<Script::NodeLessEqual>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseConcat(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseNot(TokenType ignoreType)
    {
        if(tokenType == TokenType::Not)
        {
            getToken();
            auto node = make_shared<Script::NodeNot>();
            node->args[0] = parseCompare(ignoreType);
            return insertNode(static_pointer_cast<Script::Node>(node));
        }

        return parseCompare(ignoreType);
    }

    uint32_t parseAnd(TokenType ignoreType)
    {
        uint32_t retval = parseNot(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::And)
            {
                auto node = make_shared<Script::NodeAnd>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseNot(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseXor(TokenType ignoreType)
    {
        uint32_t retval = parseAnd(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::Xor)
            {
                auto node = make_shared<Script::NodeXor>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseAnd(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseOr(TokenType ignoreType)
    {
        uint32_t retval = parseXor(ignoreType);

        while(tokenType != ignoreType)
        {
            if(tokenType == TokenType::Or)
            {
                auto node = make_shared<Script::NodeOr>();
                node->args[0] = retval;
                getToken();
                node->args[1] = parseXor(ignoreType);
                retval = insertNode(static_pointer_cast<Script::Node>(node));
            }
            else
            {
                break;
            }
        }

        return retval;
    }

    uint32_t parseConditional(TokenType ignoreType)
    {
        if(tokenType == TokenType::If)
        {
            auto cond = make_shared<Script::NodeConditional>();
            getToken();

            if(tokenType != TokenType::LParen)
            {
                errorFn(L"expected : (");
            }

            getToken();
            cond->args[0] = parseExpression(TokenType::RParen);

            if(tokenType != TokenType::RParen)
            {
                errorFn(L"expected : )");
            }

            getToken();
            cond->args[1] = parseAssign(ignoreType);
            bool gotSemicolon = false;

            if(tokenType == TokenType::Semicolon)
            {
                gotSemicolon = true;
                getToken();
            }

            if(tokenType == TokenType::Else)
            {
                getToken();
                cond->args[2] = parseAssign(ignoreType);
            }
            else
            {
                cond->args[2] = insertNode(shared_ptr<Script::Node>(new Script::NodeBlock));

                if(gotSemicolon)
                {
                    putBackToken(TokenType::Semicolon, L";");
                }
            }

            return insertNode(static_pointer_cast<Script::Node>(cond));
        }

        return parseOr(ignoreType);
    }

    uint32_t parseAssign(TokenType ignoreType)
    {
        uint32_t retval = parseConditional(ignoreType);

        if(tokenType == ignoreType)
        {
            return retval;
        }

        if(tokenType == TokenType::Assign)
        {
            shared_ptr<Script::Node> &node = script->nodes[retval];

            if(node->type() != Script::Node::Type::ReadIndex)
            {
                errorFn(L"can't assign to non-variable");
            }

            shared_ptr<Script::NodeAssignIndex> newNode = make_shared<Script::NodeAssignIndex>();
            newNode->args[0] = dynamic_cast<Script::NodeReadIndex *>(node.get())->args[0];
            newNode->args[1] = dynamic_cast<Script::NodeReadIndex *>(node.get())->args[1];
            node = newNode;
            getToken();
            newNode->args[2] = parseAssign(ignoreType);
        }

        return retval;
    }

    uint32_t parseExpression(TokenType ignoreType)
    {
        return parseAssign(ignoreType);
    }

    shared_ptr<Script> run()
    {
        getChar();
        getToken();
        uint32_t node = parseExpression(TokenType::EndOfFile);
        if(node != script->nodes.size() - 1)
        {
            auto block = make_shared<Script::NodeBlock>();
            block->nodes.push_back(node);
            node = insertNode(block);
        }

        if(tokenType != TokenType::EndOfFile)
        {
            errorFn(L"unexpected token");
        }

        return script;
    }
};
}

shared_ptr<Script> Script::parse(wstring code)
{
    Parser parser(code);
    return parser.run();
}

#if 1 // Test Script

namespace
{
initializer init([]()
{
    cout << "Enter Code (enter an eof to quit):\n";
    wstring scriptCode = L"";

    while(true)
    {
        wint_t ch = wcin.get();

        if(ch == WEOF)
        {
            break;
        }

        scriptCode += ch;
    }

    try
    {
        shared_ptr<Script> script = Script::parse(scriptCode);
        wcout << (wstring)*script->evaluate() << endl << L"No Errors." << endl;
    }
    catch(exception &e)
    {
        cout << e.what();
    }

    exit(0);
});
}

#endif // Test Script

