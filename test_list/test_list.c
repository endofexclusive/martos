/*
Copyright (c) 2014, Martin Åberg All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The names of the copyright holder(s) may not be used to endorse or
   promote products derived from this software without specific prior
   written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>
#include <stddef.h>
#include <martos/martos.h>
#include <test_common.h>

/* Code coverage test for Node and List functions. */
void test_task_f(void *user_data)
{
    List l;
    Node n1;
    Node n2;

    list_init(&l);
    n1.prio = 3;
    n1.name = "one";
    n2.prio = 8;
    n2.name = "two";

    assert(true == list_is_empty(&l));
    assert(NULL == list_get_head(&l));

    list_add_tail(&l, &n1);
    assert(false == list_is_empty(&l));
    assert(&n1 == list_get_head(&l));

    list_add_tail(&l, &n2);
    assert(&n1 == list_get_head(&l));

    assert(&n1 == list_find(&l, "one"));
    assert(&n2 == list_find(&l, "two"));

    list_rem_head(&l);
    assert(&n2 == list_get_head(&l));
    assert(NULL == list_find(&l, "one"));
    list_rem_head(&l);

    list_enqueue(&l, &n1);
    assert(&n1 == list_get_head(&l));
    list_enqueue(&l, &n2);
    assert(&n2 == list_get_head(&l));
    list_unlink(&n1);
    assert(&n2 == list_get_head(&l));

    list_enqueue(&l, &n1);
    assert(&n2 == list_get_head(&l));

    assert(&n2 == list_rem_head(&l));
    assert(&n1 == list_rem_head(&l));
    assert(NULL == list_rem_head(&l));
    assert(NULL == list_rem_head(&l));

    assert(3 == n1.prio);
    assert(8 == n2.prio);
    assert(NULL == list_find(&l, "one"));
    assert(NULL == list_find(&l, "two"));

    test_pass();
}

