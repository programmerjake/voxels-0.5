/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "stream.h"
#include <sys/types.h>
#include <unistd.h>

StreamPipe::StreamPipe()
{
    int fd[2];
    if(0 != pipe(fd))
    {
        string msg = "can't create pipe: ";
        msg += strerror(errno);
        throw new IOException(msg);
    }
    readerInternal = unique_ptr<Reader>(new FileReader(fdopen(fd[0], "r")));
    writerInternal = unique_ptr<Writer>(new FileWriter(fdopen(fd[1], "w")));
}
