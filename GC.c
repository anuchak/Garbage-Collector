#include <stdlib.h>
#include <stdio.h>

#define STACK_MAX 256
#define GC_THRESHOLD 8

typedef enum {
	OBJ_INT,
	OBJ_PAIR
} ObjectType;

typedef struct sObject {
  unsigned char marked;

  ObjectType type;

  struct sObject* next;
  // next object in the linked list of heap allocated objects

  union {
    // OBJ_INT
    int value;
    // OBJ_PAIR
    struct {
        struct sObject* head;
        struct sObject* tail;
    };
  };
} Object;

typedef struct {
	Object* stack[STACK_MAX];
	int stackSize;

        // first object in the linked list of all objects on heap
	Object* firstObject;

	// total number of currently allocated objects
	int numberOfObjects;

	// number of objects required to trigger a GC
	int maxObjects;

} VM;

void gc(VM* vm);
void objectPrint(Object* obj);

VM* newVM() {
	VM* vm = malloc(sizeof(VM));
	vm->stackSize = 0;
	vm->firstObject = NULL;
	vm->numberOfObjects = 0;
	vm->maxObjects = GC_THRESHOLD;
	return vm;
}

void assert (int check, const char* message) {
      if(!check) {
      	printf("%s\n", message);
      	exit(1);
      }
}

void push(VM* vm, Object* value) {
	assert(vm->stackSize < STACK_MAX, "Stack overflow");
        vm->stack[vm->stackSize++] = value;
}

Object* pop(VM* vm) {
    assert(vm->stackSize > 0, "Stack underflow");
    Object* ob = vm->stack[vm->stackSize-1];
    vm->stackSize--;
    return ob;
}

Object * newObject(VM* vm, ObjectType type) {
	if(vm->numberOfObjects == vm->maxObjects)
		gc(vm);
	Object* object = malloc(sizeof(Object));
	object->type = type;
	object->marked = 0;
	object->next = vm->firstObject;
	vm->firstObject = object;
	vm->numberOfObjects++;
	return object;
}

void pushInt (VM* vm, int val) {
	Object* object = newObject(vm, OBJ_INT);
	object->value = val;
	push(vm,object);
}

Object* pushPair (VM* vm) {
	Object* object = newObject(vm, OBJ_PAIR);
	object->tail = pop(vm);
	object->head = pop(vm);

	push(vm, object);
	return object;
}

void mark(Object* obj) {
       // if already marked, just return. To avoid
       // recursing on cycles in the object graph

        if(obj->marked)
		return;
	obj->marked = 1;
	if(obj->type == OBJ_PAIR)
	{
		obj->head->marked = 1;
		obj->tail->marked = 1;
	}
       
       printf("Mark -> ");
       objectPrint(obj);
       printf("\n");
}

void markAll(VM* vm) {
	for(int i = 0; i < vm->stackSize; i++)
		mark(vm->stack[i]);
}

void sweep(VM* vm) {
	Object **obj = &vm->firstObject;
	while(*obj) {
		if(!(*obj)->marked)
		{
			// This object wasn't reached, remove from list
                        // and free it.
			Object *not_reached = *obj;
                        printf("This object was reached -> ");
                        objectPrint(*obj);
			*obj = not_reached->next;
			free(not_reached);
			vm->numberOfObjects--;
                        printf("\n");
		}
		else
		{
			// This object was reached, unmark it for the
                        // next garbage collector call
			(*obj)->marked = 0;
                        printf("This object was reached -> ");
                        objectPrint(*obj);
                        (*obj) = (*obj)->next;
                        printf("\n");
		}
	}
}

void gc (VM* vm) {
	int number = vm->numberOfObjects;
        markAll(vm);
	sweep(vm);
	vm->maxObjects = vm->numberOfObjects * 2;

	printf("Collected %d objects, %d remaining.\n", number - vm->numberOfObjects,
		vm->numberOfObjects);
}

void objectPrint (Object* obj) {
	switch(obj->type)
	{
		case OBJ_INT:
		      printf("%d", obj->value);
		      break;
		case OBJ_PAIR:
		      printf("(");
		      objectPrint(obj->head);
		      printf(", ");
		      objectPrint(obj->tail);
		      printf(")");
		      break;
	}
}

void freeVM(VM* vm) {
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}

void test1() {
	printf("Test 1: Objects on stack preserved.\n");
	VM* vm = newVM();
	pushInt(vm,1);
	pushInt(vm,2);
	gc(vm);
	assert(vm->numberOfObjects == 2, "should have been preserved.");
        freeVM(vm);
}

void test2() {
	printf("Test 2: Unreachable objects are collected\n");
	VM* vm = newVM();
	pushInt(vm,1);
	pushInt(vm,2);
	pop(vm);
	pop(vm);
	gc(vm);
	assert(vm->numberOfObjects == 0, "Collected objects");
	freeVM(vm);
}

void test3() {
    printf("Test 3: Reach nested objects. \n");
    VM* vm = newVM();
    pushInt(vm,1);
    pushInt(vm,2);
    pushPair(vm);
    pushInt(vm,3);
    pushInt(vm,4);
    pushPair(vm);
    pushPair(vm);
    gc(vm);
    assert(vm->numberOfObjects == 7, "Reached the objects");
    free(vm);
}

void test4() {
    printf("Test 4: Handle cycles.\n");
    VM* vm = newVM();
    pushInt(vm,1);
    pushInt(vm,2);
    Object* a = pushPair(vm);
    pushInt(vm,3);
    pushInt(vm,4);
    Object* b = pushPair(vm);
    a->tail = b;
    b->tail = a;
    gc(vm);
    assert(vm->numberOfObjects == 4, "collected objects");
    freeVM(vm);
}

void Test() {
	printf("Test \n");
	VM* vm = newVM();
	for (int i = 0; i < 1000; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			pushInt(vm, i);
		}
	}

	for (int k = 0; k < 20; k++)
	{
		pop(vm);
	}

	freeVM(vm);
}

int main(int argc, char** argv)
{
	// test1();
    // test4();
	// test2();
	// test3();
	return 0;
}