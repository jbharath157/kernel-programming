# Mutexes

Almost all semaphores found in the Linux kernel are used for mutual exclusion by count of 1.

Using a semaphore for mutual exclusion incurs overhead, so the kernel provides a new interface: **mutex**.

The **mutex** subsystem checks and enforces the following rules:

1. **Only one task can hold the mutex at a time.**

2. **Whoever locked a mutex must unlock it.**

   - You cannot lock a mutex in one context and unlock it in another.

3. **Recursive locks and unlocks are not allowed.**

4. **Process cannot exit while holding a mutex.**

5. **Mutex cannot be acquired from an interrupt handler.**

---

# Differences between Mutexes and Semaphores

## What happens when a process tries to acquire a mutex lock?

When acquiring a mutex, there are three possible paths that can be taken:

1. **Fast Path**
2. **Mid Path**
3. **Slow Path**

The path that will be taken depends on the state of the mutex.

---

### Fast Path:

- This path is taken when **no process has acquired the mutex** yet.

### Mid Path:

- **When the mutex is not available**, the process tries to go for the mid path, also called **optimistic spinning**.
- This path is executed **only if there are no other processes ready to run with higher priority** and the **owner of the mutex is running**.
- In this path, the process tries to **spin using MCS lock**, hoping the owner will release the lock soon.
- This path **avoids expensive context switches**.

### Slow Path:

- This is the **last resort** path.
- This path behaves like a **semaphore lock**.
- If the lock cannot be acquired by the process, the task is added to a **wait queue**.
- The process will **sleep** until it is **woken up by the unlock path**.

---

# Dynamic way

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

struct mutex *mylock;

static int __init test_hello_init(void)
{
    mylock = kmalloc(sizeof(mylock), GFP_KERNEL);
    mutex_init(mylock);
    mutex_lock(mylock);
    pr_info("Starting critical region\n");
    pr_info("Ending critical region\n");
    mutex_unlock(mylock);
    kfree(mylock);
    return -1;
}

static void __exit test_hello_exit(void)
{
}

module_init(test_hello_init);
module_exit(test_hello_exit);

```

# Static way

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

DEFINE_MUTEX(mylock);

static int __init test_hello_init(void)
{
    mutex_lock(&mylock);
    pr_info("Starting critical region\n");
    pr_info("Ending critical region\n");
    mutex_unlock(&mylock);
    return -1;
}

static void __exit test_hello_exit(void)
{
}

module_init(test_hello_init);
module_exit(test_hello_exit);
```

# mutex_trylock

```c
int mutex_trylock(struct mutex *lock);
```

Tries to acquire the given mutex

Return:

	1	--> Successful
	0	--> Otherwise

# Mutex Locking Functions

## `int mutex_lock_interruptible(struct mutex *lock);`

- Places the calling process in the **`TASK_INTERRUPTIBLE`** state when it sleeps.

### Return Value:
- **0**: Mutex is successfully acquired.
- **-EINTR**: If the task receives a signal while waiting for the mutex.

---

## `int mutex_lock_killable(struct mutex *lock);`

- Places the calling process in the **`TASK_KILLABLE`** state when it sleeps. Only **fatal signals** can interrupt the process.

### Return Value:
- **0**: Mutex is successfully acquired.
- **-EINTR**: If the task receives a **fatal signal** while waiting for the mutex.


**Note:** Mutex semantics are fully enforced when CONFIG DEBUG_MUTEXES is enabled

---

# Test if the Mutex is Taken

## `int mutex_is_locked(struct mutex *lock);`

- This function checks if the mutex is currently locked.

### Return Value:
- **1**: Mutex is locked.
- **0**: Mutex is unlocked.



# Spinlock vs Mutexes

| **Requirement**                               | **Recommended Lock** |
|-----------------------------------------------|----------------------|
| Low overhead locking                          | Spinlock             |
| Short lock hold time                          | Spinlock             |
| Long lock hold time                           | Mutex                |
| Need to lock from interrupt context           | Spinlock             |
| Need to sleep while holding the lock          | Mutex                |

