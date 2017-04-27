int epicsStrGlobMatch(
    const char *str, const char *pattern)
{
    const char *cp=NULL, *mp=NULL;
    
    while ((*str) && (*pattern != '*')) {
        if ((*pattern != *str) && (*pattern != '?'))
            return 0;
        pattern++;
        str++;
    }
    while (*str) {
        if (*pattern == '*') {
            if (!*++pattern)
                return 1;
            mp = pattern;
            cp = str+1;
        }
        else if ((*pattern == *str) || (*pattern == '?')) {
            pattern++;
            str++;
        }
        else {
            pattern = mp;
            str = cp++;
        }
    }
    while (*pattern == '*')
        pattern++;
    return !*pattern;
}
