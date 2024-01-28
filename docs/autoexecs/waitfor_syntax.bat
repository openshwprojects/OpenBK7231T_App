
// Waitfor syntax samples
// WaitFor can support following operators:
// - no operator (default, it means equals)
// - operator < (less)
// - operator > (more)
// - operator ! (not requal)

// default usage - wait for NoPingTime to reach exact value 100
waitFor NoPingTime 100

// extra operator - wait for NoPingTime to become less than 100
// (this will be triggered anytime variable changes to value less than 100)
waitFor NoPingTime < 100

// extra operator - wait for NoPingTime to become more than 100
// (this will be triggered anytime variable changes to value more than 100)
waitFor NoPingTime > 100

// extra operator - wait for NoPingTime to become different than 100
// (this will be triggered anytime variable changes to value other than 100)
waitFor NoPingTime ! 100

