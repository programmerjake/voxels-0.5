#include "script.h"
#include "util.h"
#include <cwctype>

namespace
{
struct Parser
{
    shared_ptr<Script> script;
    wstring code;
    Parser(wstring code)
        : script(make_shared<Script>()), code(code)
    {
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
        EOF,
        IntegerLit,
        FloatLit,
        StringLit,
        BooleanLit,
        Id,
        LBracket,
        RBracket,
        LBrace,
        RBrace,
        If,
        Else,
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
        MakeTranslate
    };
    TokenType tokenType = Comma;
    wstring tokenText = L"";
    int tokLine, tokCol;

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
                        errorFn(L"expected : */");
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
    #error finish
    }

    shared_ptr<Script> run()
    {
        getChar();
        getToken();
        #error finish
    }
};
}

shared_ptr<Script> Script::parse(wstring code)
{
    Parser parser(code);
    return parser.run();
}
