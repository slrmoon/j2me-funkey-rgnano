/* local build helper: Ubuntu 24.04 i386 static libm lacks fmod/fmodf */
double fmod(double x, double y) {
    double r;
    __asm__ ("1: fprem\n\tfnstsw %%ax\n\tsahf\n\tjp 1b" : "=t"(r) : "0"(x), "u"(y) : "ax", "cc");
    return r;
}
float fmodf(float x, float y) { return (float) fmod((double) x, (double) y); }
