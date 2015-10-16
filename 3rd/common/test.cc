#include <stdio.h>
#include "log.h"
#include "cstr.h"
#include "array.h"
#include "value.h"

int main(int argc, char **argv) {
    printf("hello\n");
    String* shit = new String("shit");
    LOG_LOG(shit->str());

    Array *array = Array::create();
    Object *object = Object::create();
    array->add(object);
    object->release();
    array->release();

    Value value;
    value.object = NULL;
    return 0;
}
