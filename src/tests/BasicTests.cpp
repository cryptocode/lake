#include <iostream>
#include "../vmlib/Object.h"
#include "../vmlib/VM.h"
#include "../vmlib/OptParser.h"
#include <../vmlib/ExprStackOps.h>
#include "../vmlib/ExprDump.h"
#include "../vmlib/ExprExpressionList.h"
#include "../vmlib/ExprInvoke.h"
#include "../vmlib/ExprBinOp.h"
#include "../vmlib/ExprConditionalChain.h"
#include "../vmlib/ExprLoad.h"
#include "../vmlib/ExprStore.h"
#include "../vmlib/ExprUnaryOp.h"
#include "../vmlib/AsmParser.h"

using namespace std;

namespace lake {

void testUnicode()
{
    VM vm;

    ExprDump* dumpString = new ExprDump();
    dumpString->operand = new Object((char*)u8"Dump this string and ümläuts");
    vm.root->fndata->body->addExpression(dumpString);

    vm.eval();
}

void testRelOp()
{
    VM vm;

    // 2 < 3? true
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)2)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)3)));
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::less, TokenType::LessThan, TOK_LESSTHAN));
    vm.root->fndata->body->addExpression(new ExprDump());

    // 4 == 5? false
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)4)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)5)));
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::equals, TokenType::Equal, TOK_EQUAL));
    vm.root->fndata->body->addExpression(new ExprDump());

    // 2 < 3 || 4==5? true
    // Note that the < and == expressions are already on the top of stack at this point
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::logicalOr, TokenType::Or, TOK_OR));
    vm.root->fndata->body->addExpression(new ExprDump());

    vm.eval();
}

void testIfElse()
{
    VM vm;

    // Define two local const variables (known at compile-time, so no trouble that we "new Object" here
    size_t localVar44 = 0;
    size_t localVar99 = 1;
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)44)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)99)));

    // Reference the variable by name (compile-time name->stack index translation)

    // IF a < b
    auto cond = new ExprExpressionList(nullptr);
    cond->addExpression(new ExprLoad(localVar44));
    cond->addExpression(new ExprLoad(localVar99));
    cond->addExpression(new ExprBinOp(&Object::less, TokenType::LessThan, TOK_LESSTHAN));

    auto trueBranch = new ExprExpressionList(nullptr);
    {
        trueBranch->addExpression(new ExprPush(new Object((char *) u8"if branch called! setting local var to:")));
        trueBranch->addExpression(new ExprDump());

        // Update value
        trueBranch->addExpression(new ExprPush(new Object((int64_t) 123)));
        trueBranch->addExpression(new ExprStore(localVar44));
        // And read it back out and dump it to check that it's correct
        trueBranch->addExpression(new ExprLoad(localVar44));
        trueBranch->addExpression(new ExprDump());
    }


    // Else; use a named scope (closure), just for fun
    auto elseBranch = new ExprExpressionList(nullptr);
    {
        elseBranch = new ExprExpressionList(nullptr);
        elseBranch->addExpression(new ExprPush(new Object((char *) u8"else branch called!")));
        elseBranch->addExpression(new ExprDump());
    }

    auto ifStmt = new ExprConditionalChain();
    ifStmt->guard = cond;
    ifStmt->body = trueBranch;

    auto elseStmt = new ExprConditionalChain();
    elseStmt->body = elseBranch;

    // Chain
    ifStmt->nextChain = elseStmt;

    vm.root->fndata->body->addExpression(ifStmt);

    vm.eval();
}

void testSimpleMath()
{
    VM vm;

    // Must use ExprPush instead of pushing directly, since we're doing multiple operations, so that operands are pushed in the right order.

    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)21)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)7)));
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::add, TokenType::Add, TOK_AND));

    vm.root->fndata->body->addExpression(new ExprDump());

    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)4)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)5)));
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::mul, TokenType::Mul, TOK_MUL));

    vm.root->fndata->body->addExpression(new ExprDump());

    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)6)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)2)));
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::div, TokenType::Div, TOK_DIV));

    vm.root->fndata->body->addExpression(new ExprDump());

    vm.eval();
}

void testAdd()
{
    VM vm;

    // Must use ExprPush instead of pushing directly, since we're doing multiple operations, so that operands are pushed in the right order.

    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)21)));
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)7)));
    vm.root->fndata->body->addExpression(new ExprBinOp(&Object::add, TokenType::Add, TOK_ADD));

    vm.root->fndata->body->addExpression(new ExprDump());

    vm.eval();
}

void fact()
{
    // This implements factorial(n) recursively to test, well, recursion
    //
    // pseudo code for the recursive closure:
    //
    // fact(n: i64): i64
    // {
    //      if (n <= 1) return 1;
    //      return n*fact(n-1);
    // }

    VM vm;

    // Variable to hold the closure, so it can be called recursively
    size_t localFactClosureIdx = 0;
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)0)));

    // Make function instance of type fact(i64): i64
    // We use the VM stack as we're not taking saving the instance of the closure (static call)

    Object* c = new Object(new FunctionData(nullptr, "fact"));
    c->fndata->body = new ExprExpressionList(c);
    {
        // Parameter "n" index. This is relative to the stack
        // when the closure is called.
        int64_t paramN = -1;

        auto ifStmt = new ExprConditionalChain();
        ifStmt->guard = new ExprExpressionList(ifStmt);
        ifStmt->body = new ExprExpressionList(ifStmt);

        // if (n <= 1)

        // n, n-1, n-2, ...
        ifStmt->guard->addExpression(new ExprLoad(paramN, TokenType::Rel));
        ifStmt->guard->addExpression(new ExprPush(new Object((int64_t)1)));
        ifStmt->guard->addExpression(new ExprBinOp(&Object::lessEqual, TokenType::LessEqual, TOK_LESSEQUAL), DebugInfo {Location{1,0,0}});

        // return 1
        {
            // Note how "return <expr>" is simply push <expr>, exit scope

            ifStmt->body = new ExprExpressionList(ifStmt);
            ifStmt->body->addExpression(new ExprPush(new Object((int64_t)1)));
        }


        auto elseStmt = new ExprConditionalChain();
        elseStmt->guard = nullptr;
        elseStmt->body = new ExprExpressionList(nullptr);

        // Chain
        ifStmt->nextChain = elseStmt;

        // return n*fact(n-1)
        {
            elseStmt->body = new ExprExpressionList(elseStmt);

            // push n (for the upcoming multiply)
            elseStmt->body->addExpression(new ExprLoad(paramN, TokenType::Rel));

            // push n-1 as argument to fact
            elseStmt->body->addExpression(new ExprLoad(paramN, TokenType::Rel));
            elseStmt->body->addExpression(new ExprUnaryOp(&Object::inplace_decrement, TokenType::Dec, TOK_DEC));

            // push closure and call it
            elseStmt->body->addExpression(new ExprLoad(localFactClosureIdx));
            elseStmt->body->addExpression(new ExprInvoke());

            // multiply
            elseStmt->body->addExpression(new ExprBinOp(&Object::mul, TokenType::Mul, TOK_MUL), DebugInfo {Location{2,0,0}});
        }

        // The only expression on the "fact" closure is the if/else chain
        c->fndata->body->addExpression(ifStmt);

        // Squash the parameter, keep the result
        c->fndata->body->addExpression(new ExprSquash(1));
    }

    // Create the closure; put instance on stack, but no invocation yet
    vm.root->fndata->body->addExpression(new ExprPush(c));

    // Put it into the variable
    vm.root->fndata->body->addExpression(new ExprStore(localFactClosureIdx));

    // Push local argument to compute fact(<arg>)
    vm.root->fndata->body->addExpression(new ExprPush(new Object((int64_t)6)));

    // Push the closure back to the stack so we can invoke it
    vm.root->fndata->body->addExpression(new ExprLoad(localFactClosureIdx));

    // Push invoke. This will pop the create-closure expr and eval it. We should really move that into a variable.
    vm.root->fndata->body->addExpression(new ExprInvoke());

    // Show the result (6! == 720)
    vm.root->fndata->body->addExpression(new ExprDump(new Object((char*)"The factorial is? ")));
    vm.root->fndata->body->addExpression(new ExprDump());

    // Go
    vm.eval();
}
}//ns

using namespace lake;

int main(int argc, const char * argv[])
{
    lake::OptParser opt;
    opt.addOption("help", "h", "Display usage information");
    opt.addOption("trace", "t", "Trace execution level, 0 (off)..5 (high)", 1);
    opt.addOption("tracestack", "", "Display stack content");

    const char* error = opt.parse(argc, argv);
    if(error)
    {
        printf("Invalid options: %s\n", error);
        return 1;
    }

    Process::instance().debugInfo = true;

    if (opt.hasOption("trace"))
    {
        Process::instance().traceLevel = std::stoi(opt.getFirstValue("trace"));
    }

    if (opt.hasOption("tracestack"))
    {
        Process::instance().traceStack = true;
    }

    if(opt.hasOption("help"))
    {
        opt.printUsage();
        return 0;
    }

    auto start = chrono::steady_clock::now();

    try
    {
        testUnicode();
        testRelOp();
        testSimpleMath();
        testAdd();
        testIfElse();
        fact();
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    auto end = chrono::steady_clock::now();
    auto diff = end - start;

    if (Process::instance().traceLevel > 1)
    {
        std::cout << "Execution time: " << chrono::duration <double, milli> (diff).count() << " ms" << std::endl;
    }

    return 0;
}
