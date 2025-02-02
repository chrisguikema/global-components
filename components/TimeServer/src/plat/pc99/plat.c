/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>
#include <assert.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4/arch/constants.h>
#include <camkes.h>
#include <platsupport/plat/timer.h>
#include <utils/util.h>
#include <sel4utils/sel4_zf_logif.h>
#include <simple/simple.h>
#include <sel4utils/arch/tsc.h>

#include "../../time_server.h"
#include "../../plat.h"

static uint64_t tsc_frequency = 0;

uint64_t the_timer_tsc_frequency()
{
    return tsc_frequency;
}

// We declare this with a weak attribute here as we would like this component to work
// regardless of whether the assembly declared this to have a simple template or not.
// Having this as weak allows us to test for this at run time / link time
void camkes_make_simple(simple_t *simple) __attribute__((weak));

void plat_post_init(ltimer_t *ltimer, ps_irq_ops_t *irq_ops)
{
    ps_irq_t irq_info = { .type = PS_IOAPIC, .ioapic = { .ioapic = 0, .pin = 2, .level = 0, .polarity = 0, .vector = 2}};
    irq_id_t irq_id = ps_irq_register(irq_ops, irq_info, time_server_irq_handle, NULL);
    ZF_LOGF_IF(irq_id < 0, "Failed to register IRQ");

    // Attempt to detect the presence of a simple interface and try and get the
    // tsc frequency from it
    tsc_frequency = 0;
    if (camkes_make_simple) {
        simple_t simple;
        camkes_make_simple(&simple);
        tsc_frequency = x86_get_tsc_freq_from_simple(&simple);
    }

    if (tsc_frequency == 0) {
        // failed to detect from bootinfo for whatever reason, rely on the pit calibration
        tsc_frequency = ltimer_pit_get_tsc_freq(ltimer);
    }
}
