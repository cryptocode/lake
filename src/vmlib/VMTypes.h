#ifndef LAKE_VM_TYPES_H
#define LAKE_VM_TYPES_H

namespace lake {

#define TOK_NOP "nop"
#define TOK_PUSH "push"
#define TOK_POP "pop"
#define TOK_DEC "dec"
#define TOK_INC "inc"
#define TOK_MUL "mul"
#define TOK_DIV "div"
#define TOK_ADD "add"
#define TOK_SUB "sub"
#define TOK_NEGATE "neg"
#define TOK_IF "if"
#define TOK_ELSE "else"
#define TOK_REPEAT "repeat"
#define TOK_CAST "cast"
#define TOK_DUPLICATE "dup"
#define TOK_COPY "copy"
#define TOK_SWAP "swap"
#define TOK_LIFT "lift"
#define TOK_SINK "sink"
#define TOK_SQUASH "squash"
#define TOK_REMOVE "remove"
#define TOK_LESSTHAN "lt"
#define TOK_GREATERTHAN "gt"
#define TOK_LESSEQUAL "le"
#define TOK_GREATEREQUAL "ge"
#define TOK_EQUAL "eq"
#define TOK_NOTEQUAL "ne"
#define TOK_NOT "not"
#define TOK_AND "and"
#define TOK_OR "or"
#define TOK_SAME "same"
#define TOK_IS "is"
#define TOK_INVOKE "invoke"
#define TOK_TAIL "tail"
#define TOK_TYPEFUNCTION "function"
#define TOK_WITHSTACK "withstack"
#define TOK_LOADSTACK "loadstack"
#define TOK_UNLOADSTACK "unloadstack"
#define TOK_CLEAR "clear"
#define TOK_SIZE "size"
#define TOK_TYPEARRAY "array"
#define TOK_COLLGET "get"
#define TOK_COLLPUT "put"
#define TOK_COLLDEL "del"
#define TOK_COLLCONTAINS "contains"
#define TOK_COLLREVERSE "reverse"
#define TOK_FRAME "frame"
#define TOK_GC "gc"
#define TOK_EVAL "eval"
#define TOK_DEFINE "define"
#define TOK_REL "rel"
#define TOK_ABS "abs"
#define TOK_LOCAL "local"
#define TOK_ARG "arg"
#define TOK_PARENT "parent"
#define TOK_ROOT "root"
#define TOK_LOAD "load"
#define TOK_STORE "store"
#define TOK_TYPEOBJECT "object"
#define TOK_TYPEINT "int"
#define TOK_TYPEFLOAT "float"
#define TOK_TYPESTRING "string"
#define TOK_TYPECHAR "char"
#define TOK_TYPEBOOL "bool"
#define TOK_TYPEVIEWPOINTER "ptr"
#define TOK_TYPEVIEWPOINTER_UNDERSCORE "_ptr"
#define TOK_TYPEVIEWVOID "_void"
#define TOK_TYPEVIEWUINT8 "_uint8"
#define TOK_TYPEVIEWUINT16 "_uint16"
#define TOK_TYPEVIEWUINT32 "_uint32"
#define TOK_TYPEVIEWUINT64 "_uint64"
#define TOK_TYPEVIEWSINT8 "_sint8"
#define TOK_TYPEVIEWSINT16 "_sint16"
#define TOK_TYPEVIEWSINT32 "_sint32"
#define TOK_TYPEVIEWSINT64 "_sint64"
#define TOK_TYPEVIEWUCHAR "_uchar"
#define TOK_TYPEVIEWUSHORT "_ushort"
#define TOK_TYPEVIEWUINT "_uint"
#define TOK_TYPEVIEWULONG "_ulong"
#define TOK_TYPEVIEWSCHAR "_schar"
#define TOK_TYPEVIEWSSHORT "_sshort"
#define TOK_TYPEVIEWSINT "_sint"
#define TOK_TYPEVIEWSLONG "_slong"
#define TOK_TYPEVIEWFLOAT "_float"
#define TOK_TYPEVIEWDOUBLE "_double"
#define TOK_TRUE "true"
#define TOK_FALSE "false"
#define TOK_NULL "null"
#define TOK_DUMP "dump"
#define TOK_ASSERTTRUE "assert"
#define TOK_HALT "halt"
#define TOK_FFI "ffi"
#define TOK_LIB "lib"
#define TOK_SYM "sym"
#define TOK_CALL "call"
#define TOK_STRUCT "struct"
#define TOK_MODULE "module"
#define TOK_UNWIND "unwind"
#define TOK_CHECKPOINT "checkpoint"
#define TOK_TYPEUNORDEREDMAP "umap"
#define TOK_TYPEUNORDEREDSET "uset"
#define TOK_COLLFOREACH "foreach"
#define TOK_COLL "coll"
#define TOK_COLLAPPEND "append"
#define TOK_COLLINSERT "insert"
#define TOK_COLLSPREAD "spread"
#define TOK_COLLRSPREAD "rspread"
#define TOK_COLLPROJECTION "projection"
#define TOK_TYPEPAIR "pair"
#define TOK_ACCUMULATE "accumulate"
#define TOK_DEFAULTPRECISION "precision"
#define TOK_DEFAULTEPSILON "epsilon"
#define TOK_CURRENT "current"
#define TOK_STACK "stack"
#define TOK_STACKHIERARCHY "stackhierarchy"
#define TOK_FREELIST "freelist"
#define TOK_SWEEPLIST "sweeplist"
#define TOK_DTOR "dtor"
#define TOK_RESERVE "reserve"
#define TOK_COMMIT "commit"
#define TOK_COMMIT_INDEX "commitindex"
#define TOK_REVERT "revert"
#define TOK_SETCREATOR "setcreator"
#define TOK_SAVEARGS "saveargs"
#define TOK_EXPRLIST "exprlist"

/**
 * Type enumerator values
 */
enum class TokenType : uint8_t
{
    // A live object should never have zero'd members
    Bug=0,

    // Use for matching; any token is matched
    Any=10,

    // Denotes an invalid token type
    Invalid,

    // An object of this type has been GC collected
    InvalidCollected,

    // Object types. These token types doubles as runtime object types.
    TypeObject,
    TypeInt,
    TypeFloat,
    TypeString,
    TypeSymbol,
    TypeChar,
    TypeBool,
    TypeArray,
    TypeUnorderedMap,
    TypeUnorderedSet,
    TypePair,
    TypeFunction,
    TypeOperation,
    TypeExprListObject,
    TypeProjection,
    TypeFFISymbol,
    TypeFFIStruct,

    // View types (these are also tokens for FFI types)
    // Don't change the order - used in range checks

    TypeViewPointer,
    TypeViewStruct,
    TypeViewVoid,

    TypeViewUchar,
    TypeViewUshort,
    TypeViewUint,
    TypeViewUlong,

    TypeViewSchar,
    TypeViewSshort,
    TypeViewSint,
    TypeViewSlong,

    TypeViewUint8,
    TypeViewUint16,
    TypeViewUint32,
    TypeViewUint64,

    TypeViewSint8,
    TypeViewSint16,
    TypeViewSint32,
    TypeViewSint64,

    TypeViewFloat,
    TypeViewDouble,
    TypeViewLongDouble,

    // Value literals
    Null,
    True,
    False,

    // New line (only yielded if skipWS == true)
    NewLine,

    Identifier,     // [a..Z] [a..Z 0..9]*
    Minus,          // - (on its own)
    IntegerLiteral, // -? [0x|0b]* [0..9]*
    FloatLiteral,   // [0..9]* . [0..9]* "f"+
    CharLiteral,    // 'a'
    StringLiteral,  // "abc"

    BlockStart,     // {
    BlockEnd,       // }
    ParenStart,     // (
    ParenEnd,       // )

    Destructor,     // '~'

    // Operations
    Nop,

    Push,
    Pop,
    Inc,
    Dec,

    SetCreator,

    If,
    Else,
    And,
    Or,
    Not,

    Equal,
    NotEqual,
    LessThan,
    GreaterThan,
    LessEqual,
    GreaterEqual,
    // Points to the same Object
    Same,

    // push string "abc"; is string  -> true on stack
    Is,

    Negate,
    Mul,
    Div,
    Add,
    Sub,

    Accumulate,

    DefaultPrecision,
    DefaultEpsilon,

    Invoke,
    Tail,
    LoadStack,
    UnloadStack,
    Define,

    Rel,
    Abs,
    Parent,
    AbsRoot,
    Local,
    Arg,

    Load,
    Store,

    SaveArgs,

    Reserve,

    Commit,
    CommitIndex,
    Revert,

    WithStack,
    Eval,
    Repeat,
    Cast,
    Duplicate,
    Copy,
    Swap,
    Current,

    Lift,
    Sink,
    Squash,
    Remove,

    Size,
    Clear,
    Frame,
    GC,

    Coll,
    CollDel,
    CollGet,
    CollPut,
    CollAppend,
    CollInsert,
    CollContains,
    CollForeach,
    CollReverse,
    CollProjection,
    CollSpread,
    CollReverseSpread,

    Dump,
    AssertTrue,
    Halt,

    // ffi lib sym call struct
    Ffi,
    Lib,
    Sym,
    Call,
    Struct,

    Module,

    Unwind,
    Checkpoint,

    // Sentinel to indicate end of input
    EndOfStream,
};


/**
 * To avoid calling (track) more than once=>debug aid
 */
constexpr uint8_t FLAG_GC_TRACKED = 1;
/**
 * Object was marked reachable during the mark phase
 */
constexpr uint8_t FLAG_GC_REACHABLE = 2;

/**
 * If set, never ever gc/destroy. Useful for singletons and sentinels
 * (which are often be statically allocated instead of heap allocated)
 */
constexpr uint8_t FLAG_GC_PINNED = 4;

/**
 * If set, this is a destructor function
 */
constexpr uint8_t FLAG_DTOR = 8;

/**
 * The object data is allocated on the freestore and must be freed
 * when the object is destructed.
 */
constexpr uint8_t FLAG_FREESTORE = 16;

/**
 * If set, the object is null.
 */
constexpr uint8_t FLAG_ISNULL = 32;

/**
 * If set, attempting to mutate the object will lead to a runtime error
 */
constexpr uint8_t FLAG_CONST = 64;

/**
 * If set, the object's data is allocated via FFI -> don't delete data as we don't own it
 */
constexpr uint8_t FLAG_FOREIGN = 128;

}//ns

#endif // Header guard
