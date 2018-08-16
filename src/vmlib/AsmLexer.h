#ifndef LAKE_ASMLEXER_H
#define LAKE_ASMLEXER_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include "VMTypes.h"

namespace lake
{
/**
 * Maintains information about the source code location for a
 * lexeme or AST node.
 */
class Location
{
public:

    Location()
    {
        this->line = 0;
        this->col = 0;
        this->fileIndex = 0;
    }

    Location(const Location &loc) : Location(loc.line, loc.col, loc.fileIndex)
    {
    }

    Location(int line, int col, int fileIndex)
    {
        this->line = line;
        this->col = col;
        this->fileIndex = fileIndex;
    }

    bool equals(Location &other)
    {
        return fileIndex == other.fileIndex && line == other.line && col == other.col;
    }

    void update(int line, int col, int fileIndex)
    {
        this->line = line;
        this->col = col;
        this->fileIndex = fileIndex;
    }

    inline int getLine() const
    {
        return line;
    }

    inline int getCol() const
    {
        return col;
    }

    inline int getFileIndex() const
    {
        return fileIndex;
    }

private:

    int line;
    int col;
    int fileIndex;
};

/**
 * Lexical token information
 */
class Token
{
public:

    /**
     * C'tor
     *
     * Initializes all members to zero/Invalid.
     */
    Token()
    {
        type = TokenType::Invalid;
    }

    inline std::string &getLexeme()
    {
        return lexeme;
    }

    inline void setLexeme(const char ch)
    {
        this->lexeme = ch;
    }

    inline void setLexeme(const char *lexeme)
    {
        this->lexeme = lexeme;
    }

    inline void setLexeme(std::string &lexeme)
    {
        this->lexeme = lexeme;
    }

    inline void setTokenType(TokenType type)
    {
        this->type = type;
    }

    inline TokenType getType()
    {
        return type;
    }

    inline bool isViewType()
    {
        return
                type == TokenType::TypeViewPointer ||
                type == TokenType::TypeViewStruct ||
                type == TokenType::TypeViewVoid ||
                type == TokenType::TypeViewUchar ||
                type == TokenType::TypeViewUshort ||
                type == TokenType::TypeViewUint ||
                type == TokenType::TypeViewUlong ||
                type == TokenType::TypeViewSchar ||
                type == TokenType::TypeViewSshort ||
                type == TokenType::TypeViewSint ||
                type == TokenType::TypeViewSlong ||
                type == TokenType::TypeViewUint8 ||
                type == TokenType::TypeViewUint16 ||
                type == TokenType::TypeViewUint32 ||
                type == TokenType::TypeViewUint64 ||
                type == TokenType::TypeViewSint8 ||
                type == TokenType::TypeViewSint16 ||
                type == TokenType::TypeViewSint32 ||
                type == TokenType::TypeViewSint64 ||
                type == TokenType::TypeViewFloat ||
                type == TokenType::TypeViewDouble ||
                type == TokenType::TypeViewLongDouble;
    }

    inline bool isBasicTypeName()
    {
        return isNumericTypeName() ||
               type == TokenType::TypeString || type == TokenType::TypeBool ||
               type == TokenType::TypeChar || type == TokenType::TypeViewPointer;
    }

    inline bool isNumericTypeName()
    {
        return type == TokenType::TypeInt || type == TokenType::TypeFloat;
    }

    inline bool isComparison()
    {
        return type == TokenType::LessThan || type == TokenType::GreaterThan ||
               type == TokenType::LessEqual || type == TokenType::GreaterEqual ||
               type == TokenType::Equal || type == TokenType::NotEqual ||
               type == TokenType::Same || type == TokenType::Is;
    }

    inline bool isTrue()
    {
        return type == TokenType::True;
    }

    inline bool isFalse()
    {
        return type == TokenType::False;
    }

    inline void setLocation(int line, int col, int fileIndex)
    {
        location.update(line, col, fileIndex);
    }

    inline Location &getLocation()
    {
        return location;
    }

private:

    Location location;
    TokenType type;
    std::string lexeme;
};

struct TokenInfo
{
    TokenInfo(const char *lexeme, uint8_t code=0, TokenType type = TokenType::Invalid)
    {
        this->lexeme = lexeme;
        this->code = code;
        this->type = type;
    }

    // For unordered_set equality testing
    bool operator==(const TokenInfo &other) const
    {
        return (strcmp(lexeme, other.lexeme) == 0);
    }

    const char *lexeme = nullptr;
    // Binary representation
    uint8_t code = 0;
    TokenType type;
};

}//ns

// Specialize hash in the std namespace for TokenInfo; we
// need this for unordered_set caching.
namespace std
{
    template <> struct hash<lake::TokenInfo>
    {
        size_t operator()(const lake::TokenInfo& k) const
        {
            return hash<std::string>()(k.lexeme);
        }
    };
}

namespace lake {

/**
 * Lexical analyzer; turns a stream of characters into a set of
 * Token objects.
 */
class Lexer
{
public:

    /**
     * C'tor
     *
     * @param stream Stream to read from
     * @param sourceName Name of source stream. This is used to display diagnostic
     *                   messages.
     */
    Lexer(std::istream& stream, size_t fileIndex, bool skipNewLine=true);

    /**
     * Reads the next token
     *
     * @param token Receives the token
     */
    void tokenize(Token& token);

    /**
     * Bookmarks the current position. The lexer can be later rewound to the mark by
     * calling restore();
     *
     * The mark position is invalidated by calls to rewind() and restore()
     */
    void mark();

    /**
     * Rewind to the last mark position.
     */
    void restore();

    /**
     * Rewind the input stream pointer.
     *
     * @param amount Number of characters to rewind. If 0, rewind the previous
     *               token.
     */
    void rewind(int amount = 0);

    /**
     * Resets the lexer; tokenize() will start from the beginning
     * after this call. This is used to start a new pass and for
     * debugging purposes.
     */
    void reset();

    /**
     * Returns token info for the lexeme, or null if not found.
     *
     * @param lexeme Lexeme to search for, such as "<="
     * @return Operator info, or null if not found
     */
    TokenInfo* getTokenInfo(const char* lexeme);

    /**
     * A simple front end can reuse the lexer and inject extra tokens.
     */
    void expandLexer(TokenInfo info);

private:

    /**
     * Source file index, for diagnostics
     */
    size_t fileIndex;

    /**
     * Eat the next character into 'ch'. Return false if EOF.
     *
     * @return False if EOF
     */
    bool eat();

    /**
     * Current line number
     */
    int line=1;

    /**
     * Current column
     */
    int col;

    /**
     * Previous column
     */
    int prevCol;

    /**
     * Current character being lexed
     */
    char ch;

    /**
     * Current token beeing constructed. This is handed to us by the
     * caller.
     */
    Token* token;

    /**
     * Returns true if the character is an operator character
     */
    std::unordered_set<TokenInfo>::iterator getKeyword(char ch);

    /**
     * Lex number
     *
     * @param lexemeBuffer
     * @param prev
     * @param level
     */
    void lexNumber(char prev, int level);

    /**
     * Lex identifier
     *
     * @param prev
     * @param level
     */
    void lexIdentifier(char prev, int level);

    /**
     * Input stream
     */
    std::istream& input;

    /**
     * Current mark position
     */
    size_t markedPos;

    /**
     * If true, skip newlines instead of returning them as tokens
     */
    bool skipNewLine = true;
};

}//ns

#endif //LAKE_ASMLEXER_H
