#include "inhibitor.h"
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <CoreFoundation/CoreFoundation.h>

bool Inhibitor::start(bool keepDisplayOn) {
    if (active_) return true;
    CFStringRef type = keepDisplayOn
        ? kIOPMAssertionTypePreventUserIdleDisplaySleep
        : kIOPMAssertionTypePreventUserIdleSystemSleep;
    IOPMAssertionID id = 0;
    IOReturn r = IOPMAssertionCreateWithName(
        type, kIOPMAssertionLevelOn, CFSTR("keep-awake user request"), &id);
    if (r == kIOReturnSuccess) {
        assertion_ = (unsigned long)id;
        active_ = true;
        return true;
    }
    return false;
}

void Inhibitor::stop() {
    if (active_) {
        IOPMAssertionRelease((IOPMAssertionID)assertion_);
        assertion_ = 0;
        active_ = false;
    }
}
