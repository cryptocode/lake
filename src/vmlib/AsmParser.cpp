#include <iostream>
#include <fstream>
#include <strstream>
#include "AsmParser.h"
#include "VM.h"
#include "ExprStackOps.h"
#include "ExprColl.h"
#include "ExprDump.h"
#include "ExprInvoke.h"
#include "ExprBinOp.h"
#include "ExprUnaryOp.h"
#include "ExprStore.h"
#include "ExprLoad.h"
#include "ExprConditionalChain.h"
#include "ExprAssertTrue.h"
#include "ExprFFI.h"

namespace lake
{

#define DI DebugInfo(tok.getLocation())

AsmParser::AsmParser(VM& _vm) : vm(_vm)
{
    // MPIR uses localeconv for the decimal point, while we require "."
    localeInfo = localeconv();
}

void AsmParser::parse(std::string filename, ExprExpressionList* exprList)
{
    std::ifstream stream(filename, std::ios::binary);
    if (!stream.good())
        throw AsmException(std::string("Could not open assembly file for reading: ") + filename, Location(0,0,0));

    parse(stream, filename, exprList);
}

void AsmParser::parse(std::istream& stream, std::string sourcename, ExprExpressionList* exprList)
{
    fileIndex = Process::instance().filenameCount();
    Process::instance().addFilename(sourcename);

    expressionList = exprList != nullptr ? exprList : vm.root->fndata->body;

    lexer = std::make_unique<Lexer>(stream, fileIndex, false /*keep newline*/);

    parseExpressions(TokenType::EndOfStream);

    if (Process::instance().traceLevel >= Process::DEBUG)
    {
        std::cout << "Defines:" << std::endl;
        for (const auto& elem : vm.defines)
        {
            std::cout << "  " << elem.first.c_str() << "=>" << elem.second->toString().c_str() << std::endl;
        }
    }
}

void AsmParser::parseExpressions(TokenType until)
{
    while (true)
    {
        lexer->tokenize(tok);

        if (tok.getType() == until || tok.getType() == TokenType::EndOfStream)
        {
            break;
        }
        else if (tok.getType() == TokenType::NewLine || tok.getType() == TokenType::Nop)
        {
            continue;
        }
        else if (tok.getType() == TokenType::Define)
        {
            onDefine();
        }
        else if (tok.getType() == TokenType::Push)
        {
            onPush();
        }
        else if (tok.getType() == TokenType::Pop)
        {
            onPop();
        }
        else if (tok.getType() == TokenType::Load)
        {
            onLoad();
        }
        else if (tok.getType() == TokenType::Store)
        {
            onStore();
        }
        else if (tok.getType() == TokenType::Dec)
        {
            onDec();
        }
        else if (tok.getType() == TokenType::Inc)
        {
            onInc();
        }
        else if (tok.getType() == TokenType::Invoke)
        {
            onInvoke();
        }
        else if (tok.getType() == TokenType::Add)
        {
            onAdd();
        }
        else if (tok.getType() == TokenType::Sub)
        {
            onSub();
        }
        else if (tok.getType() == TokenType::Mul)
        {
            onMul();
        }
        else if (tok.getType() == TokenType::Div)
        {
            onDiv();
        }
        else if (tok.getType() == TokenType::Accumulate)
        {
            onAccumulate();
        }
        else if (tok.getType() == TokenType::DefaultPrecision)
        {
            onChangeDefaultPrecision();
        }
        else if (tok.getType() == TokenType::DefaultEpsilon)
        {
            onChangeDefaultEpsilon();
        }
        else if (tok.getType() == TokenType::Cast)
        {
            onCast();
        }
        else if (tok.getType() == TokenType::TypeFunction)
        {
            onFunctionDefinition();
        }
        else if (tok.getType() == TokenType::Or)
        {
            onLogicalOr();
        }
        else if (tok.getType() == TokenType::And)
        {
            onLogicalAnd();
        }
        else if (tok.isComparison())
        {
            onComparison();
        }
        else if (tok.getType() == TokenType::Not || tok.getType() == TokenType::Negate)
        {
            onNegate();
        }
        else if (tok.getType() == TokenType::If)
        {
            onIf();
        }
        else if (tok.getType() == TokenType::Repeat)
        {
            onRepeat();
        }
        else if (tok.getType() == TokenType::Duplicate)
        {
            onDuplicate();
        }
        else if (tok.getType() == TokenType::SetCreator)
        {
            onSetCreator();
        }
        else if (tok.getType() == TokenType::Copy)
        {
            onCopy();
        }
        else if (tok.getType() == TokenType::TypeExprListObject)
        {
            onExpressionListObject();
        }
        else if (tok.getType() == TokenType::Swap)
        {
            onSwap();
        }
        else if (tok.getType() == TokenType::Lift)
        {
            onLift();
        }
        else if (tok.getType() == TokenType::Sink)
        {
            onSink();
        }
        else if (tok.getType() == TokenType::Squash)
        {
            onSquash();
        }
        else if (tok.getType() == TokenType::Remove)
        {
            onRemove();
        }
        else if (tok.getType() == TokenType::Dump)
        {
            onDump();
        }
        else if (tok.getType() == TokenType::LoadStack)
        {
            onLoadStack();
        }
        else if (tok.getType() == TokenType::UnloadStack)
        {
            onUnloadStack();
        }
        else if (tok.getType() == TokenType::Size)
        {
            onStackSize();
        }
        else if (tok.getType() == TokenType::Clear)
        {
            onStackClear();
        }
        else if (tok.getType() == TokenType::TypeArray)
        {
            onCollection();
        }
        else if (tok.getType() == TokenType::AssertTrue)
        {
            onAssertTrue();
        }
        else if (tok.getType() == TokenType::GC)
        {
            onGC();
        }
        else if (tok.getType() == TokenType::Ffi)
        {
            onFfi();
        }
        else if (tok.getType() == TokenType::Halt)
        {
            onHalt();
        }
        else if (tok.getType() == TokenType::Unwind)
        {
            onRaise();
        }
        else if (tok.getType() == TokenType::Checkpoint)
        {
            onErrorLabel();
        }
        else if (tok.getType() == TokenType::CollForeach)
        {
            onForeach();
        }
        else if (tok.getType() == TokenType::Coll)
        {
            onCollection();
        }
        else if (tok.getType() == TokenType::Current)
        {
            onCurrent();
        }
        else if (tok.getType() == TokenType::AbsRoot)
        {
            onRoot();
        }
        else if (tok.getType() == TokenType::Parent)
        {
            onParent();
        }
        else if (tok.getType() == TokenType::Reserve)
        {
            onReserve();
        }
        else if (tok.getType() == TokenType::Commit)
        {
            onCommit();
        }
        else if (tok.getType() == TokenType::Revert)
        {
            onRevert();
        }
        else if (tok.getType() == TokenType::SaveArgs)
        {
            onSaveArgs();
        }
        else
        {
            throw AsmException("Unexpected token", tok.getLocation());
        }
    }
}

// This can be passed to match(...) as a no-op
void ignore(){}

// NOTE: http://stackoverflow.com/questions/14677997/stdfunction-vs-template
void AsmParser::match(std::initializer_list<std::pair<TokenType, std::function<void()>>> types, std::string error)
{
    lexer->tokenize(tok);

    bool found = false;
    for (auto& item : types)
    {
        if (item.first == tok.getType() || item.first == TokenType::Any)
        {
            // Call the lambda for this token type
            item.second();

            found = true;
            break;
        }
    }

    if (!found)
        throw AsmException(error, tok.getLocation());
}

std::string AsmParser::getIdentifier()
{
    lexer->tokenize(tok);

    if (tok.getType() != TokenType::Identifier)
        throw AsmException("Expected identifier (keywords are not valid identifiers)", tok.getLocation());

    return std::string(tok.getLexeme());
}

std::string& AsmParser::getStringLiteral(bool tokenize)
{
    if (tokenize)
        lexer->tokenize(tok);

    if (tok.getType() != TokenType::StringLiteral)
        throw AsmException("Expected error message (string literal) after assert", tok.getLocation());

    return tok.getLexeme();
}

// For long integer literals, only base 10. This is thus used for index purposes,
// not general numbers. Use getIntObject for that.
long AsmParser::getIntFromLiteralOrDef(bool tokenize)
{
    if (tokenize)
        lexer->tokenize(tok);

    if (tok.getType() == TokenType::IntegerLiteral)
        return std::stol(tok.getLexeme());
    else if (tok.getType() == TokenType::Identifier)
        return getDefine(false)->asLong();
    else
        throw AsmException("Expected integer or define", tok.getLocation());
}

// For arbitrary integer literals, any base
Object* AsmParser::getIntObject(bool tokenize)
{
    Object* res = nullptr;

    if (tokenize)
        lexer->tokenize(tok);

    if (tok.getType() == TokenType::IntegerLiteral)
        res = intToObject(tok.getLexeme());
    else if (tok.getType() == TokenType::Identifier)
        res = getDefine(false);
    else
        throw std::runtime_error("Not an integer or identifier");

    return res;
}

Object* AsmParser::getFloatObject(bool tokenize)
{
    Object* res = nullptr;

    if (tokenize)
        lexer->tokenize(tok);

    if (tok.getType() == TokenType::FloatLiteral)
        res = floatToObject(tok.getLexeme());
    else if (tok.getType() == TokenType::Identifier)
        res = getDefine(false);
    else
        throw std::runtime_error("Not a float or a define");

    return res;
}

Object* AsmParser::getDefine(bool tokenize)
{
    if (tokenize)
        lexer->tokenize(tok);

    auto val = vm.defines.find(tok.getLexeme());
    if (val != vm.defines.end())
        return val->second;
    else
        throw AsmException("define refers to undefined value", tok.getLocation());
}

Object* AsmParser::intToObject(std::string digits)
{
    static std::vector<Object> SMALL_CONSTANTS;
    if (SMALL_CONSTANTS.size() == 0)
    {
        SMALL_CONSTANTS.reserve(2049);

        for (int i=-1024; i <= 1024; i++)
        {
            mpz_t valInt;
            mpz_init(valInt);
            mpz_set_si(valInt, i);
            SMALL_CONSTANTS.emplace_back(valInt);
            mpz_clear(valInt);
        }
    }

    if (digits.length() <= 4)
    {
        int intval = std::stoi(digits);
        if (intval <= 1024 && intval >= -1024)
            return &SMALL_CONSTANTS[intval+1024];
    }

    Object* res = nullptr;

    mpz_t valInt;
    mpz_init(valInt);
    mpz_set_str(valInt, digits.c_str(), 0 /* use leading char to determine radix */);

    res = new Object(valInt);
    mpz_clear(valInt);

    return track(res);
}

Object* AsmParser::floatToObject(std::string value, int base)
{
    // From the docs on mpf_set_str:

    // "The decimal point expected is taken from the current locale, on systems providing localeconv."
    // Our assembly requires ".", so convert to locale before parsing.

    std::replace(value.begin(), value.end(), '.', localeInfo->decimal_point[0]);
    mpf_t valFloat;
    mpf_init(valFloat);
    mpf_set_str(valFloat, value.c_str(), base);

    Object* res = new Object(valFloat);
    mpf_clear(valFloat);

    return track(res);
}

std::tuple<Object*, TokenType> AsmParser::getTypedLiteral(bool tokenizeType)
{
    Object* res = nullptr;

    if (tokenizeType)
        lexer->tokenize(tok);

    if (tok.isBasicTypeName() || tok.getType() == TokenType::TypeFunction ||
        tok.getType() == TokenType::TypeArray || tok.getType() == TokenType::TypePair ||
        tok.getType() == TokenType::TypeUnorderedMap || tok.getType() == TokenType::TypeUnorderedSet ||
        tok.getType() == TokenType::TypeObject)
    {
        Token literalToken;
        lexer->tokenize(literalToken);

        std::string value = literalToken.getLexeme();

        if (tok.getType() == TokenType::TypeInt)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeInt>();
            else
            {
                res = intToObject(value);
            }
        }
        else if (tok.getType() == TokenType::TypeObject)
        {
            res = track(new Object(TokenType::TypeObject,  FLAG_ISNULL));
        }
        else if (tok.getType() == TokenType::TypeViewPointer)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeViewPointer>();
            else
            {
                // Reuse mpir string parsing
                auto tmp = intToObject(value);

                // Convert back to ptr
                auto ptr = tmp->fromInt<ptrdiff_t>();

                res = track(new Object(TokenType::TypeViewPointer));
                res->ptr_value = (void*) ptr;
            }
        }
        else if (tok.getType() == TokenType::TypeFloat)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeFloat>();
            else
                res = floatToObject(value);
        }
        else if (tok.getType() == TokenType::TypeBool)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeBool>();
            else
            {
                if (literalToken.isTrue())
                    res = &Object::trueObject();
                else
                    res = &Object::falseObject();
            }
        }
        else if (tok.getType() == TokenType::TypeString)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeString>();
            else
            {
                // Grab a copy
                size_t len = literalToken.getLexeme().length();
                char* copy = new char[len + 1];
                literalToken.getLexeme().copy(copy, len, 0);
                copy[len] = '\0';

                res = track(new Object(copy, FLAG_FREESTORE));
            }
        }
        else if (tok.getType() == TokenType::TypeChar)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeChar>();
            else
            {
                res = track(new Object(TokenType::TypeChar));

                res->char_value = literalToken.getLexeme().c_str()[0];
            }
        }

        else if (tok.getType() == TokenType::TypePair)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypePair>();
            else
                res = track(Object::create(new std::pair<Object*,Object*>(nullptr,nullptr)));
        }
        else if (tok.getType() == TokenType::TypeArray)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeArray>();
            else
            {
                tok = literalToken;
                long initialSize = getIntFromLiteralOrDef(false);
                std::vector<Object*>* arr = new std::vector<Object*>();
                if (initialSize > 1)
                    arr->reserve((size_t) initialSize);

                // Arrays are not literals and thus not pinned
                res = track(Object::create(arr));
            }

        }
        else if (tok.getType() == TokenType::TypeUnorderedMap)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeUnorderedMap>();
            else
            {
                tok = literalToken;
                long initialSize = getIntFromLiteralOrDef(false);
                auto map = new std::unordered_map<Object*, Object*>();
                if (initialSize > 1)
                    map->reserve((size_t) initialSize);

                // Maps are not literals and thus not pinned
                res = track(Object::create(map));
            }

        }
        else if (tok.getType() == TokenType::TypeUnorderedSet)
        {
            if (literalToken.getType() == TokenType::Null)
                res = &Object::nullObject<TokenType::TypeUnorderedMap>();
            else
            {
                tok = literalToken;
                long initialSize = getIntFromLiteralOrDef(false);
                auto set = new std::unordered_set<Object*>();
                if (initialSize > 1)
                    set->reserve((size_t) initialSize);

                // Maps are not literals and thus not pinned
                res = track(Object::create(set));
            }
        }
        else
            throw AsmException("Invalid type", tok.getLocation());
    }
    else
        throw AsmException("Invalid type expression", tok.getLocation());

    return std::make_tuple(res, tok.getType());
}

void AsmParser::onDefine()
{
    auto id = getIdentifier();

    Object* lit;
    TokenType tt;
    std::tie(lit, tt) = getTypedLiteral();

    if (Process::instance().traceLevel >= Process::DEBUG)
        std::cout << "DEFINE " << id << ":" << lit->toString().c_str() << std::endl;

    // Never ever collect defines
    lit->setFlag(FLAG_GC_PINNED);

    vm.defines[id] = lit;
}

void AsmParser::onDuplicate()
{
    static ExprDuplicate dup;

    // dup                  ; duplicate top item
    // dup ref <define>|int ; peek top stack object and put a copy in the stack location
    // dup abs <define>|int ; ...

    auto onRel = [this]()
    {
        expressionList->addExpression(track(new ExprLoad((size_t) getIntObject()->asLong(), TokenType::Rel)), DI);
    };
    auto onAbs = [this]()
    {
        expressionList->addExpression(track(new ExprLoad((size_t) getIntObject()->asLong(), TokenType::Abs)), DI);
    };

    match({std::make_pair(TokenType::NewLine, ignore),
           std::make_pair(TokenType::Rel, onRel),
           std::make_pair(TokenType::Abs, onAbs)},
          "Expected 'rel', 'abs' or newline after dup instruction");

    expressionList->addExpression(&dup, DI);
}

void AsmParser::onCopy()
{
    static ExprCopy dup;

    // copy                  ; copy top item
    // copy rel <define>|int ; pop top stack object and put a copy in stack location
    // copy abs <define>|int ; ...

    auto onRel = [this]()
    {
        expressionList->addExpression(track(new ExprLoad((size_t) getIntObject()->asLong(), TokenType::Rel)), DI);
    };
    auto onAbs = [this]()
    {
        expressionList->addExpression(track(new ExprLoad((size_t) getIntObject()->asLong(), TokenType::Abs)), DI);
    };

    match({std::make_pair(TokenType::NewLine, ignore),
           std::make_pair(TokenType::Rel, onRel),
           std::make_pair(TokenType::Abs, onAbs)},
          "Expected 'rel', 'abs' or newline after copy instruction");

    expressionList->addExpression(&dup, DI);
}

void AsmParser::onDump()
{
    static ExprDump dump;
    static ExprDumpStack dumpStack;
    static ExprDumpStackHierarchy dumpStackHierarchy;

    static ExprDumpSweepList dumpSweepList;
    static ExprDumpFreeList dumpFreeList;

    static ExprPop pop;

    auto onId = [this]()
    {
        if (tok.getLexeme().compare(TOK_STACK) == 0)
            expressionList->addExpression(&dumpStack, DI);
        else if (tok.getLexeme().compare(TOK_STACKHIERARCHY) == 0)
            expressionList->addExpression(&dumpStackHierarchy, DI);
        else if (tok.getLexeme().compare(TOK_SWEEPLIST) == 0)
            expressionList->addExpression(&dumpSweepList, DI);
        else if (tok.getLexeme().compare(TOK_FREELIST) == 0)
            expressionList->addExpression(&dumpFreeList, DI);
        else
            throw std::runtime_error("Unexpected identifier");
    };

    auto onNewLine = [this]()
    {
        expressionList->addExpression(&dump, DI);
    };

    auto onLoadOperand = [this]()
    {
        onLoad();
        expressionList->addExpression(&dump, DI);
        expressionList->addExpression(&pop);
    };

    auto otherwise = [this]()
    {
        Object* lit;
        TokenType tt;
        std::tie(lit, tt) = getTypedLiteral(false);

        // track(lit); // cannot track yet, it may be a define

        expressionList->addExpression(track(new ExprDump(lit)), DI);
    };

    match({std::make_pair(TokenType::NewLine, onNewLine),
           std::make_pair(TokenType::Identifier, onId),
           std::make_pair(TokenType::Load, onLoadOperand),
           std::make_pair(TokenType::Any, otherwise)});
}

void AsmParser::onDec()
{
    // Stateless expressions can be reused
    static ExprUnaryOp dec(&Object::inplace_decrement, TokenType::Dec, TOK_DEC);
    expressionList->addExpression(&dec, DI);
}

void AsmParser::onInc()
{
    static ExprUnaryOp inc(&Object::inplace_increment, TokenType::Inc, TOK_INC);
    expressionList->addExpression(&inc, DI);
}

void AsmParser::onInvoke()
{
    static ExprInvoke invoke(false);
    static ExprInvoke invokeTail(true);

    auto onNewLine = [this]()
    {
        expressionList->addExpression(&invoke, DI);
    };

    auto onArg = [this]()
    {
        expressionList->addExpression(&invokeTail, DI);
    };

    match({std::make_pair(TokenType::NewLine, onNewLine),
           std::make_pair(TokenType::Tail, onArg)},
          "Expected newline or 'tail' after invoke");
}

void AsmParser::tokenizeSkipNewline(Token& tok)
{
    for (;;)
    {
        lexer->tokenize(tok);

        if (tok.getType() != TokenType::NewLine)
            break;
    }
}

void AsmParser::parseBlockStart()
{
    tokenizeSkipNewline(tok);

    if (tok.getType() != TokenType::BlockStart)
        throw AsmException("Expected {", tok.getLocation());
}

void AsmParser::parseBlockEnd()
{
    tokenizeSkipNewline(tok);
    if (tok.getType() != TokenType::BlockEnd)
        throw AsmException("Expected }", tok.getLocation());
}

void AsmParser::parseBlock()
{
    parseBlockStart();
    parseExpressions(TokenType::BlockEnd);

    if (tok.getType() != TokenType::BlockEnd)
        throw AsmException("Expected }", tok.getLocation());
}

void AsmParser::parseParenStart()
{
    tokenizeSkipNewline(tok);

    if (tok.getType() != TokenType::ParenStart)
        throw AsmException("Expected (", tok.getLocation());
}

void AsmParser::parseParenEnd()
{
    tokenizeSkipNewline(tok);
    if (tok.getType() != TokenType::ParenEnd)
        throw AsmException("Expected )", tok.getLocation());
}

void AsmParser::parseParenBlock()
{
    parseParenStart();
    parseExpressions(TokenType::ParenEnd);

    if (tok.getType() != TokenType::ParenEnd)
        throw AsmException("Expected )", tok.getLocation());
}

void AsmParser::onPush()
{
    // push define <define>    ; Push define'd object
    // push <type> <value>     ; Push type/value

    lexer->tokenize(tok);

    if (tok.getType() == TokenType::Define)
    {
        expressionList->addExpression(track(new ExprPush(getDefine(true))), DI);
    }
    else if (tok.getType() == TokenType::TypeFunction)
    {
        onFunctionDefinition();
    }
    else if (tok.getType() == TokenType::TypeExprListObject)
    {
        onExpressionListObject();
    }
    else
    {
        Object* lit;
        TokenType tt;

        std::tie(lit, tt) = getTypedLiteral(false);
        expressionList->addExpression(track(new ExprPush(lit)), DI);
    }
}

void AsmParser::onPop()
{
    // pop <N=1> ; Pop, but do NOT destruct, top N items (by default 1)
    size_t count = 1;

    auto onArg = [this, &count]()
    { count = (size_t) getIntFromLiteralOrDef(false); };

    match({std::make_pair(TokenType::NewLine, ignore),
           std::make_pair(TokenType::Any, onArg)});

    expressionList->addExpression(track(new ExprPop((size_t) count)), DI);

}

void AsmParser::onRemove()
{
    // remove <N=1> ; Pop and destruct top N items (by default 1)
    size_t count = 1;

    auto onArg = [this, &count]()
    { count = (size_t) getIntFromLiteralOrDef(false); };

    match({std::make_pair(TokenType::NewLine, ignore),
           std::make_pair(TokenType::Any, onArg)});

    expressionList->addExpression(track(new ExprRemove((size_t) count)), DI);
}

void AsmParser::onStackSize()
{
    static ExprStackSize exprStackSize;
    expressionList->addExpression(&exprStackSize);
}

void AsmParser::onStackClear()
{
    static ExprStackClear clearStack(false);
    static ExprStackClear clearFrame(true);

    auto onNewLine = [this]()
    {
        expressionList->addExpression(&clearStack, DI);
    };

    auto onFrame = [this]()
    {
        expressionList->addExpression(&clearFrame, DI);
    };

    match({std::make_pair(TokenType::NewLine, onNewLine),
           std::make_pair(TokenType::Frame, onFrame)},
          "Expected newline or 'frame' after clear");
}

void AsmParser::onLoad()
{
    auto onAbs = [&]()
    {
        expressionList->addExpression(track(new ExprLoad(getIntFromLiteralOrDef(), TokenType::Abs)), DI);
    };
    auto onRel = [&]()
    {
        expressionList->addExpression(track(new ExprLoad(getIntFromLiteralOrDef(), TokenType::Rel)), DI);
    };
    auto onAbsParent = [&]()
    {
        expressionList->addExpression(
                track(new ExprLoad(getIntFromLiteralOrDef(), TokenType::Parent, getIntFromLiteralOrDef())), DI);
    };
    auto onAbsRoot = [&]()
    {
        expressionList->addExpression(
                track(new ExprLoad(getIntFromLiteralOrDef(), TokenType::AbsRoot, -1 /*root stack*/)), DI);
    };
    // Top-relative addressing
    auto onIntLiteral = [this]()
    {
        expressionList->addExpression(
                track(new ExprLoad(getIntFromLiteralOrDef(false), TokenType::IntegerLiteral)), DI);
    };
    auto onLocal = [this]()
    {
        expressionList->addExpression(
                track(new ExprLoad(getIntFromLiteralOrDef(), TokenType::Local)), DI);
    };
    auto onArg = [this]()
    {
        expressionList->addExpression(
                track(new ExprLoad(getIntFromLiteralOrDef(), TokenType::Arg)), DI);
    };
    auto onModule = [&]()
    {
        std::string moduleName = getStringLiteral();
        std::string fileName = moduleName + ".mod.lake";

        // Parse module, put in module map. If already there, grab module (to allow circular imports)
        AsmParser p(vm);
        p.parse(fileName, expressionList);
    };

    match({std::make_pair(TokenType::Abs, onAbs),
           std::make_pair(TokenType::Rel, onRel),
           std::make_pair(TokenType::Parent, onAbsParent),
           std::make_pair(TokenType::AbsRoot, onAbsRoot),
           std::make_pair(TokenType::Local, onLocal),
           std::make_pair(TokenType::Arg, onArg),
           std::make_pair(TokenType::IntegerLiteral, onIntLiteral),
           std::make_pair(TokenType::Module, onModule),
          },
          "Expected 'rel', 'abs', 'root', 'parent', 'module', 'local', 'arg' or integer literal after load instruction");
}

void AsmParser::onStore()
{
    auto onAbs = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(), TokenType::Abs)), DI);
    };
    auto onRel = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(), TokenType::Rel)), DI);
    };
    auto onAbsParent = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(), TokenType::Parent, getIntFromLiteralOrDef())), DI);
    };
    auto onAbsRoot = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(), TokenType::AbsRoot, -1 /*Root*/)), DI);
    };
    auto onCommit = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(0, TokenType::Commit)), DI);
    };
    auto onLocal = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(), TokenType::Local)), DI);
    };
    auto onArg = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(), TokenType::Arg)), DI);
    };
    // Top-relative addressing
    auto onIntLiteral = [this]()
    {
        expressionList->addExpression(
                track(new ExprStore(getIntFromLiteralOrDef(false), TokenType::IntegerLiteral)), DI);
    };

    match({std::make_pair(TokenType::Abs, onAbs),
           std::make_pair(TokenType::Rel, onRel),
           std::make_pair(TokenType::Parent, onAbsParent),
           std::make_pair(TokenType::AbsRoot, onAbsRoot),
           std::make_pair(TokenType::Local, onLocal),
           std::make_pair(TokenType::Arg, onArg),
           std::make_pair(TokenType::Commit, onCommit),
           std::make_pair(TokenType::IntegerLiteral, onIntLiteral)},
          "Expected 'rel', 'abs', 'root', 'parent', 'local', 'arg' or integer literal after store instruction");
}

void AsmParser::onFunctionDefinition()
{
    static ExprPush exprNullFunction(&Object::nullObject<TokenType::TypeFunction>());

    struct NameGenerator
    {
        static std::string next()
        {
            static int id = 0;

            return std::string("fn_") + std::to_string(++id);
        }
    };

    Object* function = track(new Object(new (vm.fnpool.malloc()) FunctionData(nullptr, "")));
    std::string id = "";

    auto onId = [this, &id]()
    { id = tok.getLexeme(); };

    // Function with no name -> generate a name
    auto onNewLine = [this, &id]()
    { id = NameGenerator::next(); };

    auto onDestructor = [this, &function, &id]()
    {
        function->setFlag(FLAG_DTOR);
        id = TOK_DTOR;
    };

    auto onWithStack = [this, &function, &id, &onId, &onNewLine]()
    {
        function->fndata->withStack = true;

        match({std::make_pair(TokenType::Identifier, onId),
               std::make_pair(TokenType::NewLine, onNewLine)});
    };

    bool done = false;
    auto onNull = [this,&done]()
    {
        this->expressionList->addExpression(
                &exprNullFunction, DI);
        done = true;
    };

    match({std::make_pair(TokenType::Identifier, onId),
           std::make_pair(TokenType::NewLine, onNewLine),
           std::make_pair(TokenType::Null, onNull),
           std::make_pair(TokenType::Destructor, onDestructor),
           std::make_pair(TokenType::WithStack, onWithStack)});

    if (done)
        return;

    function->fndata->body = (ExprExpressionList*) lake::track(new ExprExpressionList(function));
    function->fndata->name = id;

    auto oldExprList = expressionList;
    this->expressionList = function->fndata->body;

    parseBlock();

    this->expressionList = oldExprList;
    this->expressionList->addExpression(lake::track(new ExprPush(function)), DI);
}

void AsmParser::onIf()
{
    auto oldExprList = expressionList;

    // The first link in the chain is added to the current expression list
    ExprConditionalChain* first = nullptr;
    ExprConditionalChain* prev = nullptr;

    for (; ;)
    {
        auto link = new ExprConditionalChain();
        if (first == nullptr)
            first = link;
        if (prev != nullptr)
            prev->nextChain = link;

        prev = link;

        link->guard = new ExprExpressionList(link);
        link->body = new ExprExpressionList(link);

        track(link);
        track(link->guard);
        track(link->body);

        this->expressionList = link->guard;

        // Get guard operations, which must leave a boolean Object at the stack
        parseParenBlock();

        // Empty guard: ()
        if (this->expressionList->isEmpty())
            link->guard = nullptr;

        // Block of code to run if guard is true
        this->expressionList = link->body;
        parseBlock();

        // Done?
        Token nextToken;
        lexer->mark();
        tokenizeSkipNewline(nextToken);

        if (nextToken.getType() != TokenType::Else)
        {
            lexer->restore();
            break;
        }
    }

    this->expressionList = oldExprList;
    this->expressionList->addExpression(first, DI);
}

void AsmParser::onForeach()
{
    ExprCollForeach* foreach = (ExprCollForeach *) track(new ExprCollForeach());
    foreach->exprlist = (ExprExpressionList *) track(new ExprExpressionList());

    auto oldExprList = expressionList;
    expressionList = foreach->exprlist;

    parseBlock();

    expressionList = oldExprList;

    expressionList->addExpression(foreach, DI);
}

void AsmParser::onMul()
{
    static ExprBinOp mul(&Object::mul, TokenType::Mul, TOK_MUL);
    expressionList->addExpression(&mul, DI);
}

void AsmParser::onDiv()
{
    static ExprBinOp div(&Object::div, TokenType::Div, TOK_DIV);
    expressionList->addExpression(&div, DI);
}

void AsmParser::onAdd()
{
    static ExprBinOp add(&Object::add, TokenType::Add, TOK_ADD);
    expressionList->addExpression(&add, DI);
}

void AsmParser::onSub()
{
    static ExprBinOp sub(&Object::sub,TokenType::Sub, TOK_SUB);
    expressionList->addExpression(&sub, DI);
}

void AsmParser::onAccumulate()
{
    static ExprAccumulate acc;
    expressionList->addExpression(&acc, DI);
}

void AsmParser::onLogicalOr()
{
    static ExprBinOp logicalOr(&Object::logicalOr, TokenType::Or, TOK_OR);
    expressionList->addExpression(&logicalOr, DI);
}

void AsmParser::onLogicalAnd()
{
    static ExprBinOp logicalAnd(&Object::logicalAnd, TokenType::And, TOK_AND);
    expressionList->addExpression(&logicalAnd, DI);
}

void AsmParser::onComparison()
{
    static ExprBinOp equals(&Object::equals, TokenType::Equal, TOK_EQUAL);
    static ExprBinOp notEquals(&Object::notEquals, TokenType::NotEqual, TOK_NOTEQUAL);
    static ExprBinOp lessEqual(&Object::lessEqual, TokenType::LessEqual, TOK_LESSEQUAL);
    static ExprBinOp greaterEquals(&Object::greaterEqual, TokenType::GreaterEqual, TOK_GREATEREQUAL);
    static ExprBinOp less(&Object::less, TokenType::LessThan, TOK_LESSTHAN);
    static ExprBinOp greater(&Object::greater, TokenType::GreaterThan, TOK_GREATERTHAN);
    static ExprBinOp same(&Object::same, TokenType::Same, TOK_SAME, false);
    static ExprBinOp is(&Object::is, TokenType::Is, TOK_IS, false);

    if (tok.getType() == TokenType::Equal)
        expressionList->addExpression(&equals, DI);
    else if (tok.getType() == TokenType::NotEqual)
        expressionList->addExpression(&notEquals, DI);
    else if (tok.getType() == TokenType::LessEqual)
        expressionList->addExpression(&lessEqual, DI);
    else if (tok.getType() == TokenType::GreaterEqual)
        expressionList->addExpression(&greaterEquals, DI);
    else if (tok.getType() == TokenType::LessThan)
        expressionList->addExpression(&less, DI);
    else if (tok.getType() == TokenType::GreaterThan)
        expressionList->addExpression(&greater, DI);
    else if (tok.getType() == TokenType::Same)
        expressionList->addExpression(&same, DI);
    else if (tok.getType() == TokenType::Is)
        expressionList->addExpression(&is, DI);
}

// This handles both numeric negation and logical not
void AsmParser::onNegate()
{
    static ExprUnaryOp neg(&Object::inplace_negate, TokenType::Negate, TOK_NEGATE);
    expressionList->addExpression(&neg, DI);
}

void AsmParser::onAssertTrue()
{
    this->getStringLiteral();
    expressionList->addExpression(
            track(new ExprAssertTrue(tok.getLexeme())), DI);
}

void AsmParser::onSwap()
{
    static ExprSwap swap;
    expressionList->addExpression(&swap, DI);
}

void AsmParser::onRepeat()
{
    auto onNewLine = [this]()
    { this->expressionList->addExpression(&Object::repeatObject(), DI); };
    auto onIf = [this]()
    {
        lexer->tokenize(tok);

        if (tok.isTrue())
            this->expressionList->addExpression(&Object::repeatIfTrueObject(), DI);
        else if (tok.isFalse())
            this->expressionList->addExpression(&Object::repeatIfFalseObject(), DI);
        else
            throw std::runtime_error("Illegal repeat syntax");

    };

    match({std::make_pair(TokenType::NewLine, onNewLine),
           std::make_pair(TokenType::Any, onIf)});
}

void AsmParser::onLift()
{
    // lift               ; Pop the number of lifts from stack
    // lift countToLift   ; Use a define
    // lift 2             ; Use a literal
    size_t count = 0;

    auto onArg = [this, &count]()
    { count = getIntFromLiteralOrDef(false); };

    match({std::make_pair(TokenType::NewLine, ignore),
           std::make_pair(TokenType::Any, onArg)});

    expressionList->addExpression(track(new ExprLift((size_t) count)), DI);
}

void AsmParser::onSink()
{
    // "drop 1" and "drop 2" are so going to be so common they should be static instances (same for adopt)
    long val = getIntFromLiteralOrDef();
    expressionList->addExpression(track(new ExprSink((size_t) val)));
}

void AsmParser::onSquash()
{
    long count = getIntFromLiteralOrDef();

    expressionList->addExpression(track(new ExprSquash((size_t) count)), DI);
}

void AsmParser::onLoadStack()
{
    static ExprLoadStack load;
    expressionList->addExpression(&load, DI);
}

void AsmParser::onUnloadStack()
{
    static ExprUnloadStack unload;
    expressionList->addExpression(&unload, DI);
}

void AsmParser::onCollection()
{
    static ExprCollPut putParameterized(IndexType::Parameterized);
    static ExprCollPut putAppend(IndexType::Append);
    static ExprCollPut putInsert(IndexType::Insert);
    static ExprCollGet collGet;
    static ExprCollReverse collReverse;
    static ExprCollDel collDel;
    static ExprCollSize collSize;
    static ExprCollContains collContains;
    static ExprCollClear collClear;
    static ExprCollSpread spread(false);
    static ExprCollSpread reverseSpread(true);
    static ExprCollProjection collProjection;

    auto onPut = [this]() { expressionList->addExpression(&putParameterized); };
    auto onAppend = [this]() { expressionList->addExpression(&putAppend); };
    auto onInsert = [this]() { expressionList->addExpression(&putInsert); };
    auto onGet = [this]() { expressionList->addExpression(&collGet); };
    auto onDel = [this]() { expressionList->addExpression(&collDel); };
    auto onSize = [this]() { expressionList->addExpression(&collSize); };
    auto onContains = [this]() { expressionList->addExpression(&collContains); };
    auto onClear = [this]() { expressionList->addExpression(&collClear); };
    auto onSpread = [this]() { expressionList->addExpression(&spread); };
    auto onRevSpread = [this]() { expressionList->addExpression(&reverseSpread); };
    auto onReverse = [this]() { expressionList->addExpression(&collReverse); };
    auto onProjection = [this]() { expressionList->addExpression(&collProjection); };

    match({std::make_pair(TokenType::CollPut, onPut),
           std::make_pair(TokenType::CollAppend, onAppend),
           std::make_pair(TokenType::CollInsert, onInsert),
           std::make_pair(TokenType::CollGet, onGet),
           std::make_pair(TokenType::CollDel, onDel),
           std::make_pair(TokenType::CollReverse, onReverse),
           std::make_pair(TokenType::CollContains, onContains),
           std::make_pair(TokenType::Size, onSize),
           std::make_pair(TokenType::CollProjection, onProjection),
           std::make_pair(TokenType::CollSpread, onSpread),
           std::make_pair(TokenType::CollReverseSpread, onRevSpread),
           std::make_pair(TokenType::Clear, onClear)},
          "Invalid collection syntax");
}

void AsmParser::onFfi()
{
    auto onLib = [this]()
    {
        auto onNewLine = [this]()
        {
            // The alias and path will be on the stack (path first, then alias)
            expressionList->addExpression(track(new ExprFFILoad("", "")));
        };
        auto onArg = [this]()
        {

            std::string path(getStringLiteral(false));
            std::string alias(getIdentifier());
            expressionList->addExpression(track(new ExprFFILoad(path, alias)));
        };

        match({std::make_pair(TokenType::NewLine, onNewLine),
               std::make_pair(TokenType::Any, onArg)});
    };

    auto onSym = [this]()
    {
        std::string libname(getIdentifier());
        std::string symbol(getIdentifier());

        std::vector<TokenType> types;

        while (tok.getType() != TokenType::EndOfStream)
        {
            lexer->tokenize(tok);
            if (tok.getType() == TokenType::NewLine)
                break;

            if (tok.getType() == TokenType::TypeObject)
                types.push_back(TokenType::TypeObject);
            else if (tok.isViewType())
                types.push_back(tok.getType());
            else
                throw AsmException("Expected ffi compatible argument type list", tok.getLocation());
        }

        expressionList->addExpression(track(new ExprFFISym(libname, symbol, types)));
    };

    auto onStruct = [this]()
    {
        std::string structname(getIdentifier());
        std::vector<TokenType> types;

        while (tok.getType() != TokenType::EndOfStream)
        {
            lexer->tokenize(tok);
            if (tok.getType() == TokenType::NewLine)
                break;

            if (tok.isViewType())
                types.push_back(tok.getType());
            else
                throw AsmException("Expected ffi compatible struct type list", tok.getLocation());
        }

        expressionList->addExpression(track(new ExprFFIStruct{structname, types}));
    };

    static ExprFFICall ffiCall;

    auto onCall = [this]()
    {
        expressionList->addExpression(&ffiCall);
    };

    match({std::make_pair(TokenType::Lib, onLib),
           std::make_pair(TokenType::Sym, onSym),
           std::make_pair(TokenType::Struct, onStruct),
           std::make_pair(TokenType::Call, onCall)}, "Invalid FFI syntax");
}

void AsmParser::onCast()
{
    static ExprCast intCast(TokenType::TypeInt);
    static ExprCast floatCast(TokenType::TypeFloat);
    static ExprCast strCast(TokenType::TypeString);
    static ExprCast boolCast(TokenType::TypeBool);
    static ExprCast arrCast(TokenType::TypeArray);
    static ExprCast funcCast(TokenType::TypeFunction);

    lexer->tokenize(tok);

    if (tok.getType() == TokenType::TypeString)
    {
        expressionList->addExpression(&strCast, DI);
    }
    else if (tok.isNumericTypeName())
    {
        expressionList->addExpression(tok.getType() == TokenType::TypeInt ? &intCast : &floatCast, DI);
    }
    else if (tok.getType() == TokenType::TypeBool)
    {
        expressionList->addExpression(&boolCast, DI);
    }
    else if (tok.getType() == TokenType::TypeArray)
    {
        // For now, "cast array" expects a ptr on the stack, to a struct. The stack before that must be the FFI struct definition.
        // In the future "cast array N" would convert the top N items on the stack to an array.
        expressionList->addExpression(&arrCast, DI);
    }
    else if (tok.getType() == TokenType::TypeFunction)
    {
        expressionList->addExpression(&funcCast, DI);
    }
    else
    {
        throw AsmException("Unsupported argument to cast instruction", tok.getLocation());
    }
}

void AsmParser::onGC()
{
    static ExprGC gc;
    expressionList->addExpression(&gc, DI);
}

void AsmParser::onHalt()
{
    bool popExitCode = false;
    int exitCode = 0;

    auto onNewLine = [this,&popExitCode]()
    { popExitCode = true;};
    auto onArg = [this, &exitCode]()
    { exitCode = (int) getIntFromLiteralOrDef(false); };

    match({std::make_pair(TokenType::NewLine, onNewLine),
           std::make_pair(TokenType::Any, onArg)});

    static ExprHalt exprHalt;
    if (popExitCode)
        expressionList->addExpression(&exprHalt, DI);
    else
        expressionList->addExpression(track(new ExprHalt(exitCode)), DI);
}

void AsmParser::onRaise()
{
    expressionList->addExpression(&Object::raiseRequestObject());
}

void AsmParser::onErrorLabel()
{
    expressionList->addExpression(&Object::errorLabelObject());
}

void AsmParser::onChangeDefaultPrecision()
{
    long precision = getIntFromLiteralOrDef(true);
    expressionList->addExpression(track(new ExprDefaultPrecision(precision)));
}

void AsmParser::onChangeDefaultEpsilon()
{
    Object* eps = getFloatObject(true);
    expressionList->addExpression(track(new ExprDefaultEpsilon(eps)));
}

void AsmParser::onCurrent()
{
    static ExprCurrentFunction currentFunction;
    expressionList->addExpression(&currentFunction);
}

void AsmParser::onRoot()
{
    static ExprRootFunction rootFunction;
    expressionList->addExpression(&rootFunction);
}

void AsmParser::onParent()
{
    expressionList->addExpression(
            track(new ExprParentFunction(getIntFromLiteralOrDef(true))));
}

void AsmParser::onReserve()
{
    size_t count = 1;

    auto onArg = [this, &count]()
    { count = (size_t) getIntFromLiteralOrDef(false); };

    match({std::make_pair(TokenType::NewLine, ignore),
           std::make_pair(TokenType::Any, onArg)});

    if (count > 0)
    {
        expressionList->addExpression(track(new ExprReserve((size_t) count)), DI);
    }
}

void AsmParser::onCommit()
{
    static ExprCommitStack commitStack;
    expressionList->addExpression(&commitStack);
}

void AsmParser::onRevert()
{
    static ExprRevertStack revertStack;
    expressionList->addExpression(&revertStack);
}

void AsmParser::onSetCreator()
{
    // Associate nested functions lexically (for top-level functions, there's still the root function)
    // This allows capturing free (non-local) variables
    static ExprSetCreator exprSetCreator;
    this->expressionList->addExpression(&exprSetCreator);
}

void AsmParser::onSaveArgs()
{
    static ExprSaveArgs saveArgs;
    this->expressionList->addExpression(&saveArgs);
}

void AsmParser::onExpressionListObject()
{
    auto el = new ExprExpressionListObject();
    auto oldExprList = expressionList;
    this->expressionList = el->getExpressions();

    parseBlock();

    this->expressionList = oldExprList;
    this->expressionList->addExpression(lake::track(new ExprPush(el)), DI);
}

}//ns
