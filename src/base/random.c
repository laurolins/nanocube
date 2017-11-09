/*
 * Copied this rnd_ stuff from
 * http://www.eternallyconfuzzled.com/tuts/algorithms/jsw_tut_rand.aspx
 */


/* simple thread unsafe random number generator */

#define rnd_M 2147483647
#define rnd_A 16807
#define rnd_Q ( rnd_M / rnd_A )
#define rnd_R ( rnd_M % rnd_A )

static s32 rnd_state = 1;
s32 rnd_next()
{
    rnd_state = rnd_A * (rnd_state % rnd_Q) - rnd_R * (rnd_state / rnd_Q);
    if (rnd_state <= 0) {
	    rnd_state += rnd_M;
    }
    return rnd_state;
}
