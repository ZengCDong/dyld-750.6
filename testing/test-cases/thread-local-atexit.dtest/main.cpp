
// BUILD:  $CXX main.cpp -std=c++11 -o $BUILD_DIR/thread-local-atexit.exe

// RUN:  ./thread-local-atexit.exe

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "test_support.h"

// We create an A and a B.
// While destroying B we create a C
// Given that tlv_finalize has "destroy in reverse order of construction", we
// must then immediately destroy C before we destroy A to maintain that invariant

enum State {
    None,
    ConstructedA,
    ConstructedB,
    ConstructedC,
    DestroyingB,
    DestroyedA,
    DestroyedB,
    DestroyedC,
};

struct A {
    A();
    ~A();
};

struct B {
    B();
    ~B();
};

struct C {
    C();
    ~C();
};

State state;

A::A() {
    if ( state != None ) {
        FAIL("Should be in the 'None' state");
    }
    state = ConstructedA;
}

B::B() {
    if ( state != ConstructedA ) {
        FAIL("Should be in the 'ConstructedA' state");
    }
    state = ConstructedB;
}

C::C() {
    // We construct C during B's destructor
    if ( state != DestroyingB ) {
        FAIL("Should be in the 'DestroyingB' state");
    }
    state = ConstructedC;
}

// We destroy B first
B::~B() {
    if ( state != ConstructedB ) {
        FAIL("Should be in the 'ConstructedB' state");
    }
    state = DestroyingB;
    static thread_local C c;
    if ( state != ConstructedC ) {
        FAIL("Should be in the 'ConstructedC' state");
    }
    state = DestroyedB;
}

// Then we destroy C
C::~C() {
    if ( state != DestroyedB ) {
        FAIL("Should be in the 'DestroyedB' state");
    }
    state = DestroyedC;
}

// And finally destroy A
A::~A() {
    if ( state != DestroyedC ) {
        FAIL("Should be in the 'DestroyedC' state");
    }
    state = DestroyedA;
    PASS("Success");
}

static void* work(void* arg)
{
    thread_local A a;
    thread_local B b;

    return NULL;
}

int main(int argc, const char* argv[], const char* envp[], const char* apple[]) {
    pthread_t worker;
    if ( pthread_create(&worker, NULL, work, NULL) != 0 ) {
        FAIL("pthread_create");
    }

    void* dummy;
    pthread_join(worker, &dummy);

    return 0;
}

