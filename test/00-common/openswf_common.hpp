#pragma once
#include "openswf.hpp"

openswf::Stream             create_from_file(const char* path);
openswf::TagHeader  get_tag_at(openswf::Stream&, uint32_t pos);
