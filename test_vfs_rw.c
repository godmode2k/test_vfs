/*---------------------------------------------------------
Project:	test vfs read/write
Purpose:	Single-file VFS read/write (FAT16 Only)
Author:		Ho-Jung Kim (godmode2k@hotmail.com)
Date:		Since February 21, 2013
File:		test_vfs_rw.c

License:
Last Modified Date: March 4, 2013
-----------------------------------------------------------
Note:
Reference:
	- http://codeandlife.com/2012/04/02/simple-fat-and-sd-tutorial-part-1/
	- http://www.tavi.co.uk/phobos/fat.html
---------------------------------------------------------*/



//! Header and Definition
// -------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vfs_driver.h"
// -------------------------------------------------------



//! Prototype 
// -------------------------------------------------------
// -------------------------------------------------------



//! MAIN
// -------------------------------------------------------
int main(int argc, char* argv[]) {
	{
		printf( "------------------------------\n" );
		printf( "main()\n" );
		printf( "------------------------------\n" );
	}

	FILE* fpVFS = NULL;

	int i;
	PartitionTable pt[4];
	Fat16BootSector bs;


	if ( argc < 2 ) {
		fprintf( stderr, "Usage: %s [filename]\n", argv[0] );
		return -1;
	}

	fpVFS = fopen( argv[1], "rb" );

	if ( !fpVFS ) {
		fprintf( stderr, "main: [-] Open VFS file: %s\n", argv[1] );
		return -1;
	}

	for ( i = 0; i < 4; i++ ) {
		memset( (void*)&pt[i], 0x00, sizeof(PartitionTable) );
	}
	memset( (void*)&bs, 0x00, sizeof(Fat16BootSector) );

	//read_partition_table( fpVFS );
	read_boot_sector_fat16( fpVFS, pt, &bs );
	read_files_info_fat16( fpVFS, pt, bs, NULL, false );

	printf( "\nRoot directory read, now at 0x%X\n", ftell(fpVFS) );

	read_file_data_fat16( fpVFS, pt, bs, "README  ", "   " );
	//read_file_data_fat16( fpVFS, pt, bs, "README1 ", "TXT" );
	//read_file_data_fat16( fpVFS, pt, bs, "README2 ", "TXT" );
	//read_file_data_fat16( fpVFS, pt, bs, "README3 ", "TXT" );
	//read_file_data_fat16( fpVFS, pt, bs, "README4 ", "TXT" );

//! TEST
/*
	{
		unsigned long fat_start, root_start, data_start;

		// Fixed partition 0
		fseek( fpVFS, 512 * pt[0].start_sector, SEEK_SET );
		// performed in read_boot_sector_fat16()
		//fread( &bs, sizeof(Fat16BootSector), 1, fpVFS );

		// Calculate start offsets of FAT, root directory and data
		fat_start = ftell(fpVFS) + (bs.reserved_sectors-1) * bs.sector_size;
		root_start = fat_start + bs.fat_size_sectors * bs.number_of_fats * bs.sector_size;
		data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);
		printf( "FAT start at %08X, root dir at %08X, data at %08X\n", 
			fat_start, root_start, data_start );

		//fseek( fpVFS, root_start, SEEK_SET );
		//__read_file_data_fat16( fpVFS, NULL, fat_start, data_start, bs.sectors_per_cluster * 
		//			bs.sector_size, entry.starting_cluster, entry.file_size );

	}
*/




	// Release
	if ( fpVFS ) {
		fclose( fpVFS );
		fpVFS = NULL;
	}


	return 0;
}
// -------------------------------------------------------

