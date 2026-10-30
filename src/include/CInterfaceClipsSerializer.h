#pragma once

#include <stdio.h>
#include <stdint.h>

#ifndef RDF_TERMTYPE_DEFINED
#define RDF_TERMTYPE_DEFINED
typedef enum {
        URI = 0,
        BNODE = 1,
        TYPEDLITERAL = 2,
        LANGLITERAL = 3
} TERMTYPE;
#endif

typedef struct s_ClipsSerializer ClipsSerializer;

#ifdef __cplusplus
//namespace: CInterfaceOxJsonld
extern "C" {
#endif

ClipsSerializer* ClipsSerializer_new();
//void free_ClipsSerializer(ClipsSerializer*);

int64_t ClipsSerializer_add(const char* subject, uint8_t subject_type,
                const char* predicate,
                const char* object, const char* object_suffix,
                uint8_t object_type,
                const char* graph_id, uint8_t graph_type,
                ClipsSerializer*);
char* ClipsSerializer_finish(ClipsSerializer*);

#ifdef __cplusplus
}
#endif
