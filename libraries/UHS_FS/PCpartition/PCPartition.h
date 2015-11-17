/*
 *
 * Parts from
 *
 * Arduino SdFat Library
 * Copyright (C) 2012 by William Greiman
 *
 * This file is part of the Arduino SdFat Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SdFat Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef PARTITION_H
#define	PARTITION_H

#include <UHS_FS.h>

class PCPartition {
public:
        // Array of partitions.
        PCPartition();
        int Start(storage_t *sto);
        virtual ~PCPartition();

        // NULL is returned if there was a problem
        part_t * GetPart(int number);
private:
        // ZERO if all went well.
        int st;
        part_t part[4];
};

#endif	/* PARTITION_H */

