#include <stdio.h>
#include "log.h"
#include "str.h"
#include "array.h"

int main(int argc, char **argv) {
    printf("hello\n");
    String *shit = String::create("shit");
    LOG_LOG("shit");
    LOG_LOG(shit->str());

    Array *array = Array::create();
    Object *object = Object::create();
    array->add(object);
    object->release();
    array->release();
    return 0;
}
