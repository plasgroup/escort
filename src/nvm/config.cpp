#include <mutex>
#include <cstdint>
#include <cassert>

#include "config.hpp"
#include "../globalEscort.hpp"
#include "../debug.hpp"
#include "../plog_t.hpp"

using namespace Escort;

// |_dram_space|_nvm_space|_root_pointer|nvm_heap area(including roottable)|persist_num|user_num|logarea|

nvmconfig_t::nvmconfig_t(const char* nvm_path, std::size_t heap_size) {
  _tmp_space = open_nvmfile(nvm_path);

  check_dirty();
  
  create_area(2*heap_size);

  // _tmp_space points to the head pointer of log area
  // std::size_t offset_tmp_space =
  //   CACHE_LINE_SIZE + heap_size + 2*sizeof(std::uint64_t);
  // assert(_tmp_space == reinterpret_cast<std::intptr_t>(_start_address)+offset_tmp_space);

  std::string redolog_path = std::string(nvm_path) + "_redolog";
  create_redolog_area(redolog_path.c_str());
  std::string allocatorlog_path
    = std::string(nvm_path) + "_allocatorlog";
  create_allocatorlog_area(allocatorlog_path.c_str());
}

nvmconfig_t::~nvmconfig_t() {
  munmap(_start_address, _nvm_size);
}

std::intptr_t nvmconfig_t::open_nvmfile(const char* nvm_path) {
  DEBUG_PRINT("open file", nvm_path);
  
  _nvm_fd = open(nvm_path, O_CREAT | O_RDWR,
		 S_IRUSR | S_IWUSR);
  assert(_nvm_fd >= 0);
  _nvm_size = std::filesystem::file_size(nvm_path);
  DEBUG_PRINT("_nvm_size", _nvm_size >> 30, "GiB");
  _start_address = mmap(NULL, _nvm_size, PROT_WRITE | PROT_READ, MMAP_FLAG, _nvm_fd, 0);
  if(_start_address == MAP_FAILED)
    DEBUG_ERROR("cannot open nvmfile", nvm_path);

  DEBUG_PRINT("_start_address: ", _start_address);
  return reinterpret_cast<std::intptr_t>(_start_address);
}

void nvmconfig_t::check_dirty() {
  _is_dirty =
    (*(reinterpret_cast<void**>(_start_address)) != nullptr);
}

void nvmconfig_t::create_area(std::size_t heap_size) {
  DEBUG_PRINT("heap_size", heap_size);
  assert(reinterpret_cast<void*>(_tmp_space) == _start_address);

  // create |_dram_space| area
  _dram_space_addr = reinterpret_cast<void**>(_tmp_space);
  _dram_space = DRAM_BASE;
  *_dram_space_addr = reinterpret_cast<void*>(DRAM_BASE);
  _mm_clwb(_dram_space_addr);
  _tmp_space += sizeof(void*);

  // create |_nvm_space| area
  _nvm_space_addr = reinterpret_cast<void**>(_tmp_space);
  _tmp_space += sizeof(void*);

  // create |_g_epoch_pointer| area
  _g_epoch_pointer_addr = reinterpret_cast<epoch_t*>(_tmp_space);
  GlobalVariable::Epoch = _g_epoch_pointer_addr;
  _tmp_space += sizeof(epoch_t);

  _bytemap_pointer_addr = reinterpret_cast<bytemap_nvm_t**>(_tmp_space);
  _tmp_space += sizeof(bytemap_nvm_t*);
  
  // create |_root_pointer| area
  _root_pointer_addr = reinterpret_cast<roottable_t**>(_tmp_space);
  DEBUG_PRINT("first root pointer", *_root_pointer_addr);
  _tmp_space += sizeof(roottable_t*);

  // create |nvm_heap| area
  if(_tmp_space % CACHE_LINE_SIZE != 0) {
    _tmp_space += (CACHE_LINE_SIZE - _tmp_space%CACHE_LINE_SIZE);
  }
  _nvm_space = _tmp_space;
  *_nvm_space_addr = reinterpret_cast<void*>(_nvm_space);
  _mm_clwb(_nvm_space_addr);
  _tmp_space += heap_size;

  // create |persist_num| area
  _persist_num_addr = reinterpret_cast<std::uint64_t*>(_tmp_space);
  _tmp_space += sizeof(std::uint64_t);

  // create |user_num| area
  _user_num_addr = reinterpret_cast<std::uint64_t*>(_tmp_space);
  _tmp_space += sizeof(std::uint64_t);
}

void nvmconfig_t::create_redolog_area(const char* redolog_path) {
  _nvm_redolog_fd = open(redolog_path, O_CREAT | O_RDWR,
		 S_IRUSR | S_IWUSR);
  assert(_nvm_redolog_fd >= 0);
  
  _redolog_area_size = std::filesystem::file_size(redolog_path);
  _redolog_address = mmap(NULL, _redolog_area_size, PROT_WRITE|PROT_READ, MMAP_FLAG, _nvm_redolog_fd, 0);

  if(_redolog_address == MAP_FAILED)
    DEBUG_ERROR("cannot create redolog area", redolog_path);
  
  DEBUG_PRINT("_redolog_address: ", _redolog_address);
}

void nvmconfig_t::create_allocatorlog_area(const char* allocatorlog_path) {
  _nvm_allocatorlog_fd = open(allocatorlog_path, O_CREAT | O_RDWR,
			 S_IRUSR | S_IWUSR);
  assert(_nvm_allocatorlog_fd >= 0);
  _allocatorlog_area_size = std::filesystem::file_size(allocatorlog_path);
  _allocatorlog_address = mmap(NULL, _allocatorlog_area_size, PROT_WRITE|PROT_READ, MMAP_FLAG, _nvm_redolog_fd, 0);
  if(_allocatorlog_address == MAP_FAILED)
    DEBUG_ERROR("cannot create allocatorlog area", allocatorlog_path);

  DEBUG_PRINT("_allocatorlog_address: ", _allocatorlog_address);
}
