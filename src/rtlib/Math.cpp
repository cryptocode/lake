#include <cstdint>
#include "../vmlib/Object.h"

using namespace lake;

extern "C" Object* rt_math_int_pow(Object* base, Object* exp, Object* mod)
{
    mpz_t mpz;
    mpz_init(mpz);
    mpz_powm(mpz, base->mpz, exp->mpz, mod->mpz);

    Object* result = track(Object::create(mpz));

    trace_debugf("Result: %s\n", result->dump().c_str());

    return result;
}

extern "C" Object* rt_math_int_bitwise_and(Object* a, Object* b)
{
    mpz_t mpz; mpz_init(mpz);

    mpz_and(mpz, a->mpz, b->mpz);

    return track(Object::create(mpz));
}

extern "C" Object* rt_math_int_bitwise_or(Object* a, Object* b)
{
    mpz_t mpz; mpz_init(mpz);

    mpz_ior(mpz, a->mpz, b->mpz);

    return track(Object::create(mpz));
}
