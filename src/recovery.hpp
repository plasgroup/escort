#ifndef RECOVERY_HPP
#define RECOVERY_HPP
namespace Escort {
  namespace recovery {
    void replay_redo_logs();
    void copy();
    void apply_metadata();
    namespace internal {
      void replay_allocator_logs();
    }
  }
}
#endif
