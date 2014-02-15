/*

Copyright (c) 2014, Martin Åberg All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The names of the copyright holder(s) may not be used to endorse or promote
products derived from this software without specific prior written permission.

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

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <martos/martos.h>
#include "private.h"

bool msgport_init(MsgPort *const port)
{
    SignalNumber signal_bit;

    signal_bit = task_allocate_signal(-1);
    if (-1 == signal_bit) {
        return false;
    }
    port->node.name = NULL;
    port->node.prio = 0;
    port->flags = MSGPORT_SIGNAL;
    port->signal = 1 << signal_bit;
    port->signal_task = running;
    list_init((&port->message_list));
    return true;
}

Message *msgport_wait(MsgPort *const port)
{
    /* TODO: Check consistency of port->signal. */
    Message *msg;

    disable();
    while (list_is_empty(&port->message_list)) {
        task_wait(port->signal);
    }
    /* List is not empty so we can peek it. */
    msg = (Message *) port->message_list.head.next;
    enable();
    return msg;
}

Message *msgport_get(MsgPort *const port)
{
    Message *msg;

    disable();
    msg = (Message *) list_rem_head(&port->message_list);
    enable();
    return msg;
}

void msgport_put(MsgPort *const port, Message *const message)
{
    /* TODO: Check consistency of port->signal. */
    /* Assume that the port does not disappear, for example
    here. */

    /* Some rearrangements can be done to decrease time in
    the disable() state. */
    disable();
    list_add_tail(&port->message_list, (Node *) message);
    /* Assume only port owner modifies non-list fields. */
    if ((NULL != port->signal_task) &&
      (MSGPORT_SIGNAL == port->flags)) {
        /* A task exists that waits for messages on this
        port. */
        task_signal(port->signal_task, 1 << port->signal);
    }
    enable();
}

void msgport_reply(Message *const message)
{
    /* Assume that the port does not disappear, for example
    here. Furthermore, we own the message at this time so we
    may access it freely. */
    MsgPort *port;
    port = message->reply_port;
    assert(NULL != port);
    /* FIXME: Do some run-time error handling if port is
    NULL. */
    msgport_put(port, message);
}

