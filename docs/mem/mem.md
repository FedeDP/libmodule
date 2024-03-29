# Libmodule mem API

Mem API denotes symbols exposed by `libmodule_mem.so`, that refer to the memory-ref'd concept.  
It can be found in `<module/mem/mem.h>` header.  

## API

```C
typedef void (*m_ref_dtor)(void *);

void *m_mem_new(size_t size, m_ref_dtor dtor);
void *m_mem_ref(void *src);
void *m_mem_unref(void *src);
void m_mem_unrefp(void **src);
size_t m_mem_size(void *src);
```
