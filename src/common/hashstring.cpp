#include "common/hashstring.h"
#include "containers/dict.h"
#include "common/text.h"

#if _DEBUG

namespace HStr
{
    using HashText = Text<64>;

    static DictTable<64, HashKey, HashText> ms_store;

    cstr Lookup(HashKey key)
    {
        cstr result = 0;
        HashText* pText = ms_store.get(key);
        if (pText)
        {
            result = pText->value;
        }
        return result;
    }

    void Insert(HashKey key, cstrc value)
    {
        if (!value || !value[0])
        {
            return;
        }

        HashText& dst = ms_store[key];
        if (dst.value[0])
        {
            if (!StrICmp(argof(dst.value), value))
            {
                return;
            }
            DebugInterrupt();
            return;
        }

        StrCpy(argof(dst.value), value);
    }

}; // HStr

#endif // _DEBUG
