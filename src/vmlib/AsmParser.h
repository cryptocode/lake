#ifndef LAKE_ASSEMBLYPARSER_H
#define LAKE_ASSEMBLYPARSER_H

#include <string>
#include <memory>
#include <tuple>
#include <map>
#include <clocale>
#include <utility>
#include "AsmLexer.h"
#include "Process.h"

namespace lake {

class Object;
class ExprExpressionList;

/**
 * Consumes tokens and builds an AST representation of the input, ready
 * for evaluation in a VM.
 */
class AsmParser
{
public:

    AsmParser(VM& vm);

    void parse(std::string filename, ExprExpressionList* exprList = nullptr);
    void parse(std::istream& stream, std::string sourcename, ExprExpressionList* exprList = nullptr);

    Object* floatToObject(std::string value, int base=10);
    Object* intToObject(std::string digits);

private:

    VM& vm;
    size_t fileIndex;

    /**
     * The expression list to which we're currently adding expressions
     */
    ExprExpressionList* expressionList;

    lconv* localeInfo;
    std::unique_ptr<Lexer> lexer;
    Token tok;

    void parseExpressions(TokenType until);

    /**
     * Parses a type name followed by a literal or macro identifier
     *
     * @param tokenizeType If true, tokenize to find the type. If false, assume the type is already the current token.
     */
    std::tuple<Object*,TokenType> getTypedLiteral(bool tokenizeType=true);

    std::string& getStringLiteral(bool tokenize=true);
    long getIntFromLiteralOrDef(bool tokenize = true);
    Object* getIntObject(bool tokenize = true);
    Object* getFloatObject(bool tokenize = true);

    Object* getDefine(bool tokenize);
    std::string getIdentifier();

    void onDefine();
    void onPush();
    void onDump();
    void onDec();
    void onInc();
    void onInvoke();
    void onMul();
    void onDiv();
    void onAdd();
    void onSub();
    void onAccumulate();
    void onFunctionDefinition();
    void onPop();
    void onDuplicate();
    void onRepeat();
    void onComparison();
    void onNegate();
    void onAssertTrue();
    void onLogicalOr();
    void onLogicalAnd();
    void onSwap();
    void onLift();
    void onLoadStack();
    void onUnloadStack();
    void onSink();
    void onSquash();
    void onRemove();
    void onCast();
    void onLoad();
    void onStore();
    void onStackSize();
    void onStackClear();
    void onGC();
    void onCollection();
    void onHalt();
    void onFfi();
    void onCopy();
    void onRaise();
    void onErrorLabel();
    void onIf();
    void onForeach();
    void onChangeDefaultPrecision();
    void onChangeDefaultEpsilon();
    void onCurrent();
    void onReserve();
    void onCommit();
    void onRevert();
    void onParent();
    void onSetCreator();
    void onRoot();
    void onSaveArgs();
    void onExpressionListObject();

    void parseBlock();
    void parseBlockStart();
    void parseBlockEnd();
    void parseParenBlock();
    void parseParenStart();
    void parseParenEnd();

    void tokenizeSkipNewline(Token &tok);

    void match(std::initializer_list<std::pair<TokenType, std::function<void()>>> types, std::string error="Unexpected input");
};

}//ns

#endif //LAKE_ASSEMBLYPARSER_H
