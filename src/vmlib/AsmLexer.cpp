#include "AsmLexer.h"
#include "Exceptions.h"

#include <fstream>
#include <iomanip>

namespace lake
{

std::unordered_set<TokenInfo> cache
        {
                {"{",                            128, TokenType::BlockStart},
                {"}",                            129, TokenType::BlockEnd},
                {"(",                            130, TokenType::ParenStart},
                {")",                            131, TokenType::ParenEnd},

                {TOK_NOP,                        132, TokenType::Nop},
                {TOK_PUSH,                       133, TokenType::Push},
                {TOK_POP,                        134, TokenType::Pop},
                {TOK_DEC,                        135, TokenType::Dec},
                {TOK_INC,                        136, TokenType::Inc},
                {TOK_MUL,                        137, TokenType::Mul},
                {TOK_DIV,                        138, TokenType::Div},
                {TOK_ADD,                        139, TokenType::Add},
                {TOK_SUB,                        140, TokenType::Sub},
                {TOK_NEGATE,                     141, TokenType::Negate},
                {TOK_IF,                         143, TokenType::If},
                {TOK_ELSE,                       144, TokenType::Else},
                {TOK_REPEAT,                     145, TokenType::Repeat},
                {TOK_CAST,                       146, TokenType::Cast},
                {TOK_DUPLICATE,                  147, TokenType::Duplicate},
                {TOK_COPY,                       148, TokenType::Copy},
                {TOK_SWAP,                       149, TokenType::Swap},
                {TOK_LIFT,                       150, TokenType::Lift},
                {TOK_SINK,                       151, TokenType::Sink},
                {TOK_SQUASH,                     152, TokenType::Squash},
                {TOK_REMOVE,                     153, TokenType::Remove},
                {TOK_LESSTHAN,                   154, TokenType::LessThan},
                {TOK_GREATERTHAN,                155, TokenType::GreaterThan},
                {TOK_LESSEQUAL,                  156, TokenType::LessEqual},
                {TOK_GREATEREQUAL,               157, TokenType::GreaterEqual},
                {TOK_EQUAL,                      158, TokenType::Equal},
                {TOK_NOTEQUAL,                   159, TokenType::NotEqual},
                {TOK_NOT,                        160, TokenType::Not},
                {TOK_AND,                        161, TokenType::And},
                {TOK_OR,                         162, TokenType::Or},
                {TOK_SAME,                       163, TokenType::Same},
                {TOK_IS,                         164, TokenType::Is},
                {TOK_INVOKE,                     165, TokenType::Invoke},
                {TOK_TAIL,                       166, TokenType::Tail},
                {TOK_TYPEFUNCTION,               167, TokenType::TypeFunction},
                {TOK_WITHSTACK,                  168, TokenType::WithStack},
                {TOK_LOADSTACK,                  169, TokenType::LoadStack},
                {TOK_UNLOADSTACK,                170, TokenType::UnloadStack},
                {TOK_CLEAR,                      171, TokenType::Clear},
                {TOK_SIZE,                       172, TokenType::Size},
                {TOK_TYPEARRAY,                  173, TokenType::TypeArray},
                {TOK_COLLGET,                    174, TokenType::CollGet},
                {TOK_COLLPUT,                    175, TokenType::CollPut},
                {TOK_COLLDEL,                    176, TokenType::CollDel},
                {TOK_COLLCONTAINS,               176, TokenType::CollContains},
                {TOK_FRAME,                      177, TokenType::Frame},
                {TOK_GC,                         178, TokenType::GC},
                {TOK_EVAL,                       179, TokenType::Eval},
                {TOK_DEFINE,                     180, TokenType::Define},
                {TOK_REL,                        181, TokenType::Rel},
                {TOK_ABS,                        182, TokenType::Abs},
                {TOK_PARENT,                     183, TokenType::Parent},
                {TOK_ROOT,                       184, TokenType::AbsRoot},
                {TOK_LOAD,                       185, TokenType::Load},
                {TOK_STORE,                      186, TokenType::Store},
                {TOK_TYPEOBJECT,                 187, TokenType::TypeObject},
                {TOK_TYPEINT,                    188, TokenType::TypeInt},
                {TOK_TYPEFLOAT,                  189, TokenType::TypeFloat},
                {TOK_TYPESTRING,                 190, TokenType::TypeString},
                {TOK_TYPECHAR,                   191, TokenType::TypeChar},
                {TOK_TYPEBOOL,                   192, TokenType::TypeBool},
                {TOK_TYPEVIEWPOINTER,            193, TokenType::TypeViewPointer},
                {TOK_TYPEVIEWPOINTER_UNDERSCORE, 194, TokenType::TypeViewPointer},
                {TOK_TYPEVIEWVOID,               195, TokenType::TypeViewVoid},
                {TOK_TYPEVIEWUINT8,              196, TokenType::TypeViewUint8},
                {TOK_TYPEVIEWUINT16,             197, TokenType::TypeViewUint16},
                {TOK_TYPEVIEWUINT32,             198, TokenType::TypeViewUint32},
                {TOK_TYPEVIEWUINT64,             199, TokenType::TypeViewUint64},
                {TOK_TYPEVIEWSINT8,              200, TokenType::TypeViewSint8},
                {TOK_TYPEVIEWSINT16,             201, TokenType::TypeViewSint16},
                {TOK_TYPEVIEWSINT32,             202, TokenType::TypeViewSint32},
                {TOK_TYPEVIEWSINT64,             203, TokenType::TypeViewSint64},
                {TOK_TYPEVIEWUCHAR,              204, TokenType::TypeViewUchar},
                {TOK_TYPEVIEWUSHORT,             205, TokenType::TypeViewUshort},
                {TOK_TYPEVIEWUINT,               206, TokenType::TypeViewUint},
                {TOK_TYPEVIEWULONG,              207, TokenType::TypeViewUlong},
                {TOK_TYPEVIEWSCHAR,              208, TokenType::TypeViewSchar},
                {TOK_TYPEVIEWSSHORT,             209, TokenType::TypeViewSshort},
                {TOK_TYPEVIEWSINT,               210, TokenType::TypeViewSint},
                {TOK_TYPEVIEWSLONG,              211, TokenType::TypeViewSlong},
                {TOK_TYPEVIEWFLOAT,              212, TokenType::TypeViewFloat},
                {TOK_TYPEVIEWDOUBLE,             213, TokenType::TypeViewDouble},
                {TOK_TRUE,                       214, TokenType::True},
                {TOK_FALSE,                      215, TokenType::False},
                {TOK_NULL,                       216, TokenType::Null},
                {TOK_DUMP,                       217, TokenType::Dump},
                {TOK_ASSERTTRUE,                 218, TokenType::AssertTrue},
                {TOK_HALT,                       219, TokenType::Halt},
                {TOK_FFI,                        220, TokenType::Ffi},
                {TOK_LIB,                        221, TokenType::Lib},
                {TOK_SYM,                        222, TokenType::Sym},
                {TOK_CALL,                       223, TokenType::Call},
                {TOK_STRUCT,                     224, TokenType::Struct},
                {TOK_MODULE,                     225, TokenType::Module},
                {TOK_UNWIND,                     226, TokenType::Unwind},
                {TOK_CHECKPOINT,                 227, TokenType::Checkpoint},
                {TOK_TYPEUNORDEREDMAP,           228, TokenType::TypeUnorderedMap},
                {TOK_TYPEUNORDEREDSET,           229, TokenType::TypeUnorderedSet},
                {TOK_COLLFOREACH,                230, TokenType::CollForeach},
                {TOK_COLL,                       231, TokenType::Coll},
                {TOK_COLLAPPEND,                 232, TokenType::CollAppend},
                {TOK_COLLINSERT,                 233, TokenType::CollInsert},
                {TOK_COLLSPREAD,                 234, TokenType::CollSpread},
                {TOK_COLLRSPREAD,                235, TokenType::CollReverseSpread},
                {TOK_TYPEPAIR,                   236, TokenType::TypePair},
                {TOK_ACCUMULATE,                 237, TokenType::Accumulate},
                {TOK_DEFAULTPRECISION,           238, TokenType::DefaultPrecision},
                {TOK_DEFAULTEPSILON,             239, TokenType::DefaultEpsilon},
                {TOK_DTOR,                       240, TokenType::Destructor},
                {TOK_CURRENT,                    241, TokenType::Current},
                {TOK_RESERVE,                    242, TokenType::Reserve},
                {TOK_COMMIT,                     243, TokenType::Commit},
                {TOK_COMMIT_INDEX,               244, TokenType::CommitIndex},
                {TOK_REVERT,                     245, TokenType::Revert},
                {TOK_LOCAL,                      246, TokenType::Local},
                {TOK_ARG,                        247, TokenType::Arg},
                {TOK_SETCREATOR,                 248, TokenType::SetCreator},
                {TOK_SAVEARGS,                   249, TokenType::SaveArgs},
                {TOK_COLLREVERSE,                250, TokenType::CollReverse},
                {TOK_COLLPROJECTION,             251, TokenType::CollProjection},
                {TOK_EXPRLIST,                   252, TokenType::TypeExprListObject},
        };

Lexer::Lexer(std::istream& stream, size_t fileIndex, bool skipNewLine)
        : input(stream), skipNewLine(skipNewLine)
{
    reset();
    this->fileIndex = fileIndex;
}

bool Lexer::eat()
{
    prevCol = col;
    col++;

    bool res = (bool) (input >> std::noskipws >> ch);

    if (ch == '\n')
    {
        col = 1;
        line++;
    }

    return res;
}

void Lexer::expandLexer(TokenInfo info)
{
    // Replace
    cache.erase(info);
    cache.insert(info);
}

std::unordered_set<TokenInfo>::iterator Lexer::getKeyword(char ch)
{
    return cache.find(TokenInfo(std::string(1, ch).c_str()));
}

TokenInfo* Lexer::getTokenInfo(const char* lexeme)
{
    auto match = cache.find(TokenInfo(lexeme));
    return (match == cache.end()) ? nullptr : const_cast<TokenInfo*>(&(*match));
}

void Lexer::tokenize(Token& _token)
{
    bool keepEating = true;

    this->token = &_token;
    this->token->getLexeme().clear();

    if (this->input.eof())
    {
        token->setTokenType(TokenType::EndOfStream);
        return;
    }

    decltype(getKeyword(' ')) itr;

    while (keepEating && eat())
    {
        // Semicolon outside literals is the same as a newline
        if (ch == ';')
        {
            ch = '\n';
        }

        if (ch == '\r')
        {
            continue;
        }

        if (ch == '\n')
        {
            if (skipNewLine)
                continue;

            token->setTokenType(TokenType::NewLine);
            token->setLexeme("\n");
            return;
        }

        // Skip line comments
        if (ch == '#')
        {
            char prevch = ch;
            while (eat())
            {
                // "#!" is a debugging aid; the rest of the source file is ignored
                if (ch == '!' && prevch == '#')
                {
                    token->setTokenType(TokenType::EndOfStream);
                    return;
                }

                if (ch == '\n')
                    break;

                prevch = ch;
            }
        }

        // Eat white spaces
        if (isspace(ch))
            continue;

        // Integer, Floating point or Scientific notation
        if (isdigit(ch) || ch == '-')
        {
            lexNumber(ch, 1);
            if (token->getLexeme().length() == 1 && token->getLexeme().at(0) == '-')
            {
                token->setTokenType(TokenType::Minus);
            }
            keepEating = false;
        }
        else if (ch == '\'')
        {
            if (eat())
            {
                token->setLexeme(ch);

                eat();

                if (ch != '\'')
                    throw AsmException("Expected end of character literal", Location(line, col, fileIndex));

                token->setTokenType(TokenType::CharLiteral);

                keepEating = false;
            }
            else
            {
                throw AsmException("Could not read character literal", Location(line, col, fileIndex));
            }
        }
            // String literal
        else if (ch == '"')
        {
            std::string str;

            while (keepEating && eat())
            {
                if (ch == '\r' || ch == '\n')
                {
                    throw AsmException("Unexpected end of line in string literal", Location(line, col, fileIndex));
                }

                if (ch == '"')
                {
                    break;
                }

                str += ch;
            }

            token->setTokenType(TokenType::StringLiteral);
            token->setLexeme(str);

            keepEating = false;
        }
        else if((itr = getKeyword(ch)) != cache.end())
        {
            const TokenInfo* op = &(*itr);
            token->setTokenType(op->type);

            keepEating = false;
        }
        else
        {
            lexIdentifier(ch, 1);
            keepEating = false;
        }

        token->setLocation(line, col - token->getLexeme().length(), fileIndex);
    }

    if (keepEating && token->getLexeme().size() == 0)
    {
        token->setTokenType(TokenType::EndOfStream);
    }
}

void Lexer::lexNumber(char prev, int level)
{
    token->getLexeme() = prev;

    bool done = false;
    bool seenDot = false;

    // This may change to float types
    TokenType ttype = TokenType::IntegerLiteral;

    while (!done && eat())
    {
        // Discard digit separator
        if (ch == '_')
            continue;

        // The parser will validate more closely; just make sure we accept hex and binary prefixes (0x 0X 0b 0B)
        if (isdigit(ch) || ch == 'x' || ch == 'X' || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
        {
            token->getLexeme() += ch;
        }
        else if ((ch == '-' || ch == 'e' || ch == '^') && seenDot) // scientific e notation
        {
            // 'e' works for small radixes, but @ works for all, so change it
            if (ch == 'e' || ch == '^')
                ch = '@';

            token->getLexeme() += ch;
        }
        else if (ch == '.' && !seenDot)
        {
            token->getLexeme() += ch;

            ttype = TokenType::FloatLiteral;
            seenDot = true;
        }
        else
        {
            done = true;
            rewind(1);
        }
    }

    token->setTokenType(ttype);
}

void Lexer::lexIdentifier(char prev, int level)
{
    token->getLexeme() = prev;

    bool done = false;
    while (!done && eat())
    {
        // Only space, ; ( ) { } \n \r are invalid identifier. This allows considerable
        // name generation flexibility in frontends
        if (ch != ' ' && ch != ';' && ch != '('  && ch != ')'  && ch != '{'  && ch != '}' && ch != '\n' && ch != '\r')
        {
            token->getLexeme() += ch;
        }
        else
        {
            done = true;
            rewind(1);
        }
    }

    auto ti = getTokenInfo(token->getLexeme().c_str());

    if (ti != nullptr)
        token->setTokenType(ti->type);
    else
        token->setTokenType(TokenType::Identifier);
}

void Lexer::mark()
{
    markedPos = input.tellg();
}

void Lexer::restore()
{
    size_t cur = input.tellg();

    if (cur < markedPos)
        throw AsmException("Invalid marker position", Location(line, col, fileIndex));
    else
    {
        rewind(cur - markedPos);
        markedPos = 0;
    }
}

void Lexer::rewind(int amount)
{
    if (amount == 0 && token != 0)
        amount = token->getLexeme().length();

    if (ch == '\n')
    {
        // If we rewind more than once, make sure the ch == '\n' fails
        // after the first time to keep the line count in sync. Note that
        // this works only because we never ever rewind across more than
        // one line.
        ch = 0;

        assert(line > 1);
        line--;
        col = prevCol;
    }
    else
        col -= amount;

    input.clear();
    input.seekg(-amount, std::ios_base::cur);
}

void Lexer::reset()
{
    ch = 0;
    col = 1;
    line = 1;
    prevCol = 1;
    markedPos = 0;

    input.clear();
    input.seekg(0, std::ios_base::beg);
}

}// ns