#ifdef ANDROID_WORKAROUNDS

#define pthread_cancel(x) /**/
#define to_string(x) my_to_string(x)

inline string my_to_string(int x) {
    char buf[32];
    snprintf(buf,32,"%d",x);
    return buf;
}
inline string my_to_string(double x) {
    char buf[32];
    snprintf(buf,32,"%lf",x);
    return buf;
}

#endif
