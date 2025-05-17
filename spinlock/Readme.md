# Spinlocks

The most common lock in the Linux kernel is the spin lock.

Spinlocks are used to protect short code sections that comprise just a few C statements and are therefore quickly executed and exited.

A spin lock is a lock that can be held by at most one thread of execution.

---

## When the thread tries to acquire lock which is already held?

The thread busy loops/spins waiting for the lock to become available.

## When the thread tries to acquire lock which is available?

The thread acquires the lock and continues.

---

# Spinlock Methods

## Header File:
```c
#include <linux/spinlock.h>
```

## Data Structure:
```c
spinlock_t
```

## Methods

```c
DEFINE_SPINLOCK(my_lock);  // Equivalent to:
                           // spinlock_t my_lock = __SPIN_LOCK_UNLOCKED(my_lock);
```

From `<linux/spinlock_types.h>`:
```c
#define DEFINE_SPINLOCK(x) spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
```

### 🔒 To lock a spin lock:
```c
void spin_lock(spinlock_t *lock);
```

### 🔓 To unlock a spin lock:
```c
void spin_unlock(spinlock_t *lock);
```


# Static way

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

DEFINE_SPINLOCK(my_lock);

static int __init test_hello_init(void)
{
    spin_lock(&my_lock);
    pr_info("Starting critical region\n");
    pr_info("Ending critical region\n");
    spin_unlock(&my_lock);
    return -1;
}

static void __exit test_hello_exit(void)
{
}

module_init(test_hello_init);
module_exit(test_hello_exit);
```
---

# Dynamic way

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
spinlock_t *my_lock;

static int __init test_hello_init(void)
{
    my_lock = kmalloc(sizeof(spinlock_t), GFP_KERNEL);
    spin_lock_init(my_lock);
    spin_lock(my_lock);
    pr_info("Starting critical region\n");
    pr_info("Ending critical region\n");
    spin_unlock(my_lock);
    kfree(my_lock);
    return -1;
}

static void __exit test_hello_exit(void)
{
}

module_init(test_hello_init);
module_exit(`test_hello_exit);

```

---


# Will spin lock exist on uniprocessor machines?

## When CONFIG_PREEMPT is not set / kernel preemption disabled

Spinlocks are defined as **empty operations** because critical sections cannot be entered by several CPUs at the same time on a uniprocessor system.

## When CONFIG_PREEMPT is set

In order to protect critical sections from **kernel preemption** (even on uniprocessor), spinlocks are redefined as:

```c
spin_lock   = preempt_disable();
spin_unlock = preempt_enable();
```

This ensures that even on a **uniprocessor**, the code within the spinlock is not preempted by another kernel thread.

---

## Note:

What happens if i acquire a lock which is already held by the same CPU?

**Spin locks are not recursive**

Unlike spin lock implementations in other operating systems and threading libraries, the Linux kernel’s spin locks are not recursive

This means that if you attempt to acquire a lock you already hold, you will spin, waiting for yourself to release the lock.

But because you are busy spinning, you will never release the lock and you will deadlock.

---

## spin_trylock

```c
spin_trylock()

int spin_trylock(spinlock_t *lock);
```

Tries to acquire given lock;

	If not available, returns zero.

	If available, it obtains the lock and returns nonzero

---

# Can I use spinlock when the resource is shared between kernel control path in process context vs interrupt context?

## Scenario

1. Your driver is executing and has taken a lock.
2. The device the driver is handling issues an interrupt.
3. The interrupt handler also tries to obtain the **same lock**.

### ❗ Problem:
This can lead to a **deadlock** when the interrupt handler runs on the **same processor** where the driver (process context) is already holding the lock.

- The **interrupt handler spins forever** waiting for the lock.
- But the **kernel code that holds the lock can't run**, because the interrupt preempted it.

---

## ✅ Solution:

### Disable interrupts before acquiring the spinlock, and enable them back after releasing it.

Linux provides an interface that **disables interrupts and acquires the spinlock** safely.

### Example:
```c
DEFINE_SPINLOCK(my_lock);
unsigned long flags;

spin_lock_irqsave(&my_lock, flags);
/* critical region ... */
spin_unlock_irqrestore(&my_lock, flags);
```

---

## ❓ Why is the `flags` argument needed?

### Case:
What if interrupts were **already disabled** before acquiring the spinlock?

- If we don't save the state (i.e., don't use `flags`), calling `spin_unlock_irqrestore()` would **blindly re-enable interrupts** — which may **violate the original state**.

### ✅ `spin_lock_irqsave()`:
- **Saves the current interrupt state** into `flags`.
- **Disables interrupts locally**.
- **Acquires the spinlock**.

### ✅ `spin_unlock_irqrestore()`:
- **Releases the spinlock**.
- **Restores** interrupts to the **previous state** saved in `flags`.

---

## ⚠️ What if you know interrupts are enabled?

If you are certain that **interrupts are always enabled**, you can use:

```c
DEFINE_SPINLOCK(mr_lock);
spin_lock_irq(&mr_lock);
/* critical section ... */
spin_unlock_irq(&mr_lock);
```

However...

> **Using `spin_lock_irq()` is discouraged**, because it’s difficult to guarantee that **interrupts are always enabled** in all code paths.

✅ The safer and more general solution is to use:
```c
spin_lock_irqsave();
spin_unlock_irqrestore();
```

---

# Is the kernel preemption disabled when the spinlock is acquired?

Yes. Any time kernel code holds a spinlock, **preemption is disabled** on the **current processor**.

This is necessary to ensure that:

- The current thread cannot be preempted while holding the lock.
- Another thread on the same CPU cannot be scheduled and try to re-acquire the same spinlock, which would lead to a **deadlock**.

## 🔧 Locking Behavior

- `spin_lock()` → **Disables kernel preemption**
- `spin_unlock()` → **Enables kernel preemption back**

This ensures **mutual exclusion** even on **preemptible kernels**, and it's why spinlocks are safe for **short atomic critical sections**.

---

# Important Points with Spinlocks

1. **If a lock is acquired but never released, the system is rendered unusable.**

   - All processors — including the one that acquired the lock — will eventually need to enter the critical region.
   - They spin endlessly waiting for the lock to be released.
   - If the lock is never released, this results in a **deadlock** and system stall.

2. **Spinlocks must not be held for long durations.**

   - Other processors waiting for the lock become **idle and unproductive**.
   - This severely affects system responsiveness and overall performance.

3. **Code protected by spinlocks must not sleep.**

   - You must ensure that **no function inside the spinlocked region can sleep**.
   - Example: **Do NOT call `kmalloc()` with `GFP_KERNEL`**, as it may sleep if memory is low.
   - Sleeping while holding a spinlock can cause **deadlocks** or **kernel warnings** like:
     ```
     BUG: sleeping function called from invalid context
     ```

> ✅ Always use `GFP_ATOMIC` inside spinlocked code if memory allocation is required.

