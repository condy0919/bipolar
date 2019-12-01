//! Thread Safety Analysis
//!
//! https://clang.llvm.org/docs/ThreadSafetyAnalysis.html

#ifndef BIPOLAR_CORE_THREAD_SAFETY_HPP_
#define BIPOLAR_CORE_THREAD_SAFETY_HPP_

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined(__clang__)
#define BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(x) __attribute__((x))
#else
#define BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(x) // no-op
#endif

/// BIPOLAR_CAPABILITY
///
/// `BIPOLAR_CAPABILITY` is an attribute on classes, which specifies that
/// objects of the class can be used as a capability. The string argument
/// specifies the kind of capability in error messages, e.g. "mutex".
///
/// ```
/// class BIPOLAR_CAPABILITY("mutex") Mutex {
/// public:
///   void Lock() BIPOLAR_ACQUIRE();
///
///   void ReaderLock() BIPOLAR_ACQUIRE_SHARED();
///
///   void Unlock() BIPOLAR_RELEASE();
///
///   void ReaderUnlock() BIPOLAR_RELEASE_SHARED();
///
///   bool TryLock() BIPOLAR_TRY_ACQUIRE();
///
///   bool ReaderTryLock() BIPOLAR_TRY_ACQUIRE_SHARED();
///
///   void AssertHeld() BIPOLAR_ASSERT_CAPABILITY();
///
///   void AssertReaderHeld() BIPOLAR_ASSERT_SHARED_CAPABILITY();
/// };
/// ```
#define BIPOLAR_CAPABILITY(x)                                                  \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(capability(x))

/// BIPOLAR_SCOPED_CAPABILITY
///
/// BIPOLAR_SCOPED_CAPABILITY is an attribute on classes that implement
/// RAII-style locking, in which a capability is acquired in the constructor,
/// and released in the destructor. Such classes require special handling
/// because the constructor and destructor refer to the capability via different
/// names;
///
/// ```
/// class BIPOLAR_SCOPED_CAPABILITY MutexLocker {
/// public:
///   MutexLocker(Mutex* mu) BIPOLAR_ACQUIRE(mu) : mut(mu) {
///     mu->Lock();
///   }
///
///   ~MutexLocker() BIPOLAR_RELEASE() {
///     mut->Unlock();
///   }
///
/// private:
///   Mutex* mut;
/// };
/// ```
#define BIPOLAR_SCOPED_CAPABILITY                                              \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(scoped_lockable)

/// BIPOLAR_GUARDED_BY
///
/// `BIPOLAR_GUARDED_BY` is an attribute on data members, which declares that
/// the data member is protected by the given capability. Read operations on the
/// data require shared access, while write operations require exclusive access.
///
/// ```
/// Mutex mut;
/// int val BIPOLAR_GUARDED_BY(mut);
///
/// void test() {
///   val = 0;        // Warning!
/// }
/// ```
#define BIPOLAR_GUARDED_BY(x)                                                  \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(guarded_by(x))

/// BIPOLAR_PT_GUARDED_BY
///
/// `BIPOLAR_PT_GUARDED_BY` is similar to `BIPOLAR_GUARDED_BY`, but is intended
/// for use on pointers and smart pointers. There is no constraint on the data
/// member itself, but the data that it points to is protected by the given
/// capability.
///
/// ```
/// Mutex mut;
/// int* p BIPOLAR_PT_GUARDED_BY(mut);
///
/// void test() {
///   *p = 0;           // Warning!
/// }
/// ```
///
/// A pointer can also be annotated with `GUARDED_BY`.
///
/// ```
/// Mutex mut1, mut2;
/// // `p` is guarded by `mut1`, `*p` is guarded by `mut2`.
/// int* p BIPOLAR_GUARDED_BY(mut1) BIPOLAR_PT_GUARDED_BY(mut2);
/// ```
#define BIPOLAR_PT_GUARDED_BY(x)                                               \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(pt_guarded_by(x))

/// BIPOLAR_ACQUIRED_BEFORE / BIPOLAR_ACQUIRED_AFTER
///
/// `BIPOLAR_ACQUIRED_BEFORE` and `BIPOLAR_ACQUIRED_AFTER` are attributes on
/// member declarations, specifically declarations of mutexes or other
/// capabilities. These declarations enforce a particular order in which the
/// mutexes must be acquired, in order to prevent deadlock.
///
/// ```
/// Mutex m1;
/// Mutex m2 BIPOLAR_ACQUIRED_AFTER(m1);
///
/// // Alternative declaration
/// // Mutex m2;
/// // Mutex m1 ACQUIRED_BEFORE(m2);
///
/// void foo() {
///   m2.Lock();
///   m1.Lock();  // Warning!  m2 must be acquired after m1.
///   m1.Unlock();
///   m2.Unlock();
/// }
/// ```
#define BIPOLAR_ACQUIRED_BEFORE(...)                                           \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(acquired_before(__VA_ARGS__))

#define BIPOLAR_ACQUIRED_AFTER(...)                                            \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(acquired_after(__VA_ARGS__))

/// BIPOLAR_REQUIRES / BIPOLAR_REQUIRES_SHARED
///
/// `BIPOLAR_REQUIRES` is an attribute on functions or methods, which declares
/// that the calling thread must have exclusive access to the given
/// capabilities. More than one capability may be specified. The capabilities
/// must be held on entry to the function, and must still be held on exit.
///
/// `BIPOLAR_REQUIRES_SHARED` is similar, but requires only shared access.
///
/// ```
/// Mutex mu1, mu2;
/// int a BIPOLAR_GUARDED_BY(mu1);
/// int b BIPOLAR_GUARDED_BY(mu2);
///
/// void foo() BIPOLAR_REQUIRES(mu1, mu2) {
///   a = 0;
///   b = 0;
/// }
///
/// int bar() BIPOLAR_REQUIRES_SHARED(mu1, mu2) {
///   int ret = a + b;
///   return ret;
/// }
///
/// void test1() {
///   mu1.Lock();
///   foo();         // Warning!  Requires mu2.
///   mu1.Unlock();
/// }
///
/// void test2() {
///   mu1.Lock();
///   bar();         // Warning!  Requires mu2.
///   mu1.Unlock();
/// }
/// ```
#define BIPOLAR_REQUIRES(...)                                                  \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(requires_capability(__VA_ARGS__))

#define BIPOLAR_REQUIRES_SHARED(...)                                           \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(requires_shared_capability(__VA_ARGS__))

/// BIPOLAR_ACQUIRE / BIPOLAR_ACQUIRE_SHARED
///
/// `BIPOLAR_ACQUIRE` is an attribute on functions or methods, which declares
/// that the function acquires a capability, but does not release it. The caller
/// must not hold the given capability on entry, and it will hold the capability
/// on exit.
///
/// `BIPOLAR_ACQUIRE_SHARED` is similar.
///
/// ```
/// Mutex mu;
/// MyClass myObject BIPOLAR_GUARDED_BY(mu);
///
/// void lockAndInit() BIPOLAR_ACQUIRE(mu) {
///   mu.Lock();
///   myObject.init();
/// }
///
/// void cleanupAndUnlock() BIPOLAR_RELEASE(mu) {
///   myObject.cleanup();
/// }                          // Warning!  Need to unlock mu.
///
/// void test() {
///   lockAndInit();
///   myObject.doSomething();
///   cleanupAndUnlock();
///   myObject.doSomething();  // Warning, mu is not locked.
/// }
/// ```
///
/// If no argument is passed to `BIPOLAR_ACQUIRE` or `BIPOLAR_ACQUIRE_SHARED`,
/// then the argument is assumed to be this, and the analysis will not check the
/// body of the function. This pattern is intended for use by classes which hide
/// locking details behind an abstract interface. For example:
///
/// ```
/// template <class T>
/// class BIPOLAR_CAPABILITY("mutex") Container {
/// private:
///   Mutex mu;
///   T* data;
///
/// public:
///   // Hide mu from public interface.
///   void Lock()   BIPOLAR_ACQUIRE() { mu.Lock(); }
///   void Unlock() BIPOLAR_RELEASE() { mu.Unlock(); }
///
///   T& getElem(int i) { return data[i]; }
/// };
///
/// void test() {
///   Container<int> c;
///   c.Lock();
///   int i = c.getElem(0);
///   c.Unlock();
/// }
/// ```
#define BIPOLAR_ACQUIRE(...)                                                   \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(acquire_capability(__VA_ARGS__))

#define BIPOLAR_ACQUIRE_SHARED(...)                                            \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(acquire_shared_capability(__VA_ARGS__))

/// BIPOLAR_RELEASE / BIPOLAR_RELEASE_SHARED
///
/// `BIPOLAR_RELEASE` and `BIPOLAR_RELEASE_SHARED` declare that the function
/// releases the given capability. The caller must hold the capability on entry,
/// and will no longer hold it on exit. It does not matter whether the given
/// capability is shared or exclusive.
///
/// If no argument is passed to `BIPOLAR_RELEASE` or `BIPOLAR_RELEASE_SHARED`,
/// then the argument is assumed to be this, and the analysis will not check the
/// body of the function. This pattern is intended for use by classes which hide
/// locking details behind an abstract interface. For example:
#define BIPOLAR_RELEASE(...)                                                   \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(release_capability(__VA_ARGS__))

#define BIPOLAR_RELEASE_SHARED(...)                                            \
    THREAD_ANNOTATION_ATTRIBUTE(release_shared_capability(__VA_ARGS__))

/// BIPOLAR_TRY_ACQUIRE / BIPOLAR_TRY_ACQUIRE_SHARED
///
/// These are attributes on a function or method that tries to acquire the given
/// capability, and returns a boolean value indicating success or failure. The
/// first argument must be true or false, to specify which return value
/// indicates success, and the remaining arguments are interpreted in the same
/// way as `BIPOLAR_ACQUIRE`.
#define BIPOLAR_TRY_ACQUIRE(...)                                               \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(try_acquire_capability(__VA_ARGS__))

#define BIPOLAR_TRY_ACQUIRE_SHARED(...)                                        \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(                                       \
        try_acquire_shared_capability(__VA_ARGS__))

/// BIPOLAR_EXCLUDES
///
/// `BIPOLAR_EXCLUDES` is an attribute on functions or methods, which declares
/// that the caller must not hold the given capabilities. This annotation is
/// used to prevent deadlock. Many mutex implementations are not re-entrant, so
/// deadlock can occur if the function acquires the mutex a second time.
///
/// ```
/// Mutex mu;
/// int a BIPOLAR_GUARDED_BY(mu);
///
/// void clear() BIPOLAR_EXCLUDES(mu) {
///   mu.Lock();
///   a = 0;
///   mu.Unlock();
/// }
///
/// void reset() {
///   mu.Lock();
///   clear();          // Warning!  Caller cannot hold 'mu'.
///   mu.Unlock();
/// }
/// ```
#define BIPOLAR_EXCLUDES(...)                                                  \
    THREAD_ANNOTATION_ATTRIBUTE(locks_excluded(__VA_ARGS__))

/// BIPOLAR_ASSERT_CAPABILITY / BIPOLAR_ASSERT_SHARED_CAPABILITY
///
/// These are attributes on a function or method that does a run-time test to
/// see whether the calling thread holds the given capability. The function is
/// assumed to fail (no return) if the capability is not held.
#define BIPOLAR_ASSERT_CAPABILITY(x)                                           \
    THREAD_ANNOTATION_ATTRIBUTE(assert_capability(x))

#define BIPOLAR_ASSERT_SHARED_CAPABILITY(x)                                    \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(assert_shared_capability(x))

/// BIPOLAR_RETURN_CAPABILITY
///
/// `BIPOLAR_RETURN_CAPABILITY` is an attribute on functions or methods, which
/// declares that the function returns a reference to the given capability. It
/// is used to annotate getter methods that return mutexes.
///
/// ```
/// class MyClass {
/// private:
///   Mutex mu;
///   int a GUARDED_BY(mu);
///
/// public:
///   Mutex* getMu() RETURN_CAPABILITY(mu) { return &mu; }
///
///   // analysis knows that getMu() == mu
///   void clear() REQUIRES(getMu()) { a = 0; }
/// };
/// ```
#define BIPOLAR_RETURN_CAPABILITY(x)                                           \
    THREAD_ANNOTATION_ATTRIBUTE(lock_returned(x))

/// BIPOLAR_NO_THREAD_SAFETY_ANALYSIS
///
/// `BIPOLAR_NO_THREAD_SAFETY_ANALYSIS` is an attribute on functions or methods,
/// which turns off thread safety checking for that method. It provides an
/// escape hatch for functions which are either
///
/// 1. deliberately thread-unsafe,
/// 2. are thread-safe, but too complicated for the analysis to understand.
///
/// ```
/// class Counter {
///   Mutex mu;
///   int a BIPOLAR_GUARDED_BY(mu);
///
///   void unsafeIncrement() BIPOLAR_NO_THREAD_SAFETY_ANALYSIS { a++; }
/// };
/// ```
///
/// Unlike the other attributes, `BIPOLAR_NO_THREAD_SAFETY_ANALYSIS` is not part
/// of the interface of a function, and should thus be placed on the function
/// definition (in the .cc or .cpp file) rather than on the function declaration
/// (in the header).
#define BIPOLAR_NO_THREAD_SAFETY_ANALYSIS                                      \
    BIPOLAR_THREAD_ANNOTATION_ATTRIBUTE(no_thread_safety_analysis)

#endif
