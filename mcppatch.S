#	WUPInstaller - Install Wii U NUS content - [https://github.com/crediar/wupinstaller]
#
# Copyright (c) 2015 crediar
#
#	WUPInstaller is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#include <asm.h>

mflr 0
stwu 1, -0x18(1)
stw  0,  0x1C(1)
stw  31, 0x14(1)
stw  30, 0x10(1)
stw  29, 0x0C(1)
stw  28, 0x08(1)

nop
cmpwi 3, 2
bne end

# check install done value
nop
nop
lwz		4,	0(3)
cmpwi	4,	2
beq		end

# set install done value
li		4,	2
stw		4,	0(3)

#IOS_Open(/dev/mcp)
nop
nop
li 4, 0
nop
mr 30, 3

#alloc( 0x24, 32 )
li 3, 0x24
li 4, 32
nop
mr 31, 3

#MCP get info
mr 3, 30
mr 5, 31
nop
nop
nop
cmpwi 3, 0
bne error

# check title IDs

#game update
lwz		3,	0(31)
subis	3,	3,	0x0005
cmpwi	3,	0x000E
beq		install

#version title
lwz		3,	4(31)
subis	3,	3,	0x1004

#JAP
cmpwi	3,	0x1000
beq		install

#USA
cmpwi	3,	0x1100
beq		install

#EUR
cmpwi	3,	0x1200
beq		install

li		3, -1

b error_title

install:
#MCP Install
mr 3, 30
mr 5, 31
nop
nop
nop
cmpwi 3, 0
bne error

loop:
li	3, 1
stw 3, 0(31)

#MCP Progress
mr 3, 30
mr 4, 31
nop
cmpwi 3, 0
bne error

lwz 3, 0(31)
cmpwi 3, 0
bne loop

#MCPGetLastError
nop
cmpwi 3, 0
bne error_title

# "Install OK"
nop
nop
b print

error:
#MCPGetLastError
nop
error_title:
mr		6,	3
addi	3,	1,	0x40
li		4,	0x20
nop
nop
#
nop
addi	3,	1,	0x40
print:
nop

end:
lwz 28, 8(1)
lwz 29, 0xC(1)
lwz 0,  0x1C(1)
lwz 30, 0x10(1)
mtlr 0
lwz 31, 0x14(1)
addi 1, 1, 0x18
blr
