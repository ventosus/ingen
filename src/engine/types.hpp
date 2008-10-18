/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstddef> // for NULL, size_t, etc

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;

typedef float    Sample;
typedef uint32_t SampleCount;
typedef uint32_t SampleRate;
typedef uint32_t FrameTime;

#endif // TYPES_HPP
