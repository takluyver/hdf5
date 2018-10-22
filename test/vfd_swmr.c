/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/***********************************************************
*
* Test program:	 
*
* Tests the VFD SWMR Feature.
*
*************************************************************/

#include "h5test.h"

/*
 * This file needs to access private information from the H5F package.
 */

#define H5F_FRIEND		    /*suppress error about including H5Fpkg	  */
#define H5F_TESTING
#include "H5FDprivate.h"
#include "H5Fpkg.h"

#include "H5CXprivate.h"    /* API Contexts                         */

#define FS_PAGE_SIZE    512
#define FILENAME        "vfd_swmr_file.h5"
#define MD_FILENAME     "vfd_swmr_metadata_file"

/* test routines for VFD SWMR */
static unsigned test_fapl(void);
static unsigned test_file_end_tick(void);
static unsigned test_file_fapl(void);
static unsigned test_writer_md(void);
static unsigned test_writer_update_md(void);


/*-------------------------------------------------------------------------
 * Function:    test_fapl()
 *
 * Purpose:     A) Verify that invalid info set in the fapl fails
 *                 as expected (see the RFC for VFD SWMR):
 *                 --version: should be a known version
 *                 --tick_len: should be >= 0
 *                 --max_lag: should be >= 3
 *                 --md_pages_reserved: should be >= 1
 *                 --md_file_path: should contain the metadata file path (POSIX)
 *              B) Verify that info set in the fapl is retrieved correctly.
 *
 * Return:      0 if test is sucessful
 *              1 if test fails
 *
 * Programmer:  Vailin Choi; July 2018
 *
 *-------------------------------------------------------------------------
 */
static unsigned
test_fapl(void)
{
    hid_t fapl = -1;    /* File access property list */
    H5F_vfd_swmr_config_t *my_config = NULL;   /* Configuration for VFD SWMR */
    herr_t ret;         /* Return value */

    TESTING("Configure VFD SWMR with fapl");

    /* Allocate memory for the configuration structure */
    if((my_config = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    HDmemset(my_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Get a copy of the file access property list */
    if((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        TEST_ERROR;

    /* Should get invalid VFD SWMR config info */
    if(H5Pget_vfd_swmr_config(fapl, my_config) < 0)
        TEST_ERROR;

    /* Verify that the version is incorrect */
    if(my_config->version >= H5F__CURR_VFD_SWMR_CONFIG_VERSION)
        TEST_ERROR;

    /* Should fail: version is 0 */
    H5E_BEGIN_TRY {
        ret = H5Pset_vfd_swmr_config(fapl, my_config);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Set valid version */
    my_config->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    /* Should fail: tick_len is -1 */
    my_config->tick_len = -1;
    H5E_BEGIN_TRY {
        ret = H5Pset_vfd_swmr_config(fapl, my_config);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Set valid tick_len */
    my_config->tick_len = 3; 
    /* Should fail: max_lag is 2 */
    my_config->max_lag = 2;
    H5E_BEGIN_TRY {
        ret = H5Pset_vfd_swmr_config(fapl, my_config);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Set valid max_lag */
    my_config->max_lag = 3;
    /* Should fail: md_pages_reserved is 0 */
    H5E_BEGIN_TRY {
        ret = H5Pset_vfd_swmr_config(fapl, my_config);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Set valid md_pages_reserved */
    my_config->md_pages_reserved = 2;
    /* Should fail: empty md_file_path */
    H5E_BEGIN_TRY {
        ret = H5Pset_vfd_swmr_config(fapl, my_config);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Set md_file_path */
    HDstrcpy(my_config->md_file_path, MD_FILENAME);
    my_config->vfd_swmr_writer = TRUE;

    /* Should succeed in setting the configuration info */
    if(H5Pset_vfd_swmr_config(fapl, my_config) < 0)
        TEST_ERROR;

    /* Clear the configuration structure */
    HDmemset(my_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Retrieve the configuration info just set */
    if(H5Pget_vfd_swmr_config(fapl, my_config) < 0)
        TEST_ERROR;

    /* Verify the configuration info */
    if(my_config->version < H5F__CURR_VFD_SWMR_CONFIG_VERSION)
        TEST_ERROR;
    if(my_config->md_pages_reserved != 2)
        TEST_ERROR;
    if(HDstrcmp(my_config->md_file_path, MD_FILENAME) != 0)
        TEST_ERROR;

    /* Close the file access property list */
    if(H5Pclose(fapl) < 0)
        FAIL_STACK_ERROR;

    if(my_config)
        HDfree(my_config);

    PASSED()
    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl);
    } H5E_END_TRY;
    if(my_config)
        HDfree(my_config);
    return 1;
} /* test_fapl() */


/*-------------------------------------------------------------------------
 * Function:    test_file_fapl()
 *
 * Purpose:     A) Verify that page buffering and paged aggregation
 *                 have to be enabled for a file to be configured
 *                 with VFD SWMR.
 *              B) Verify the VFD SWMR configuration set in fapl
 *                 used to create/open the file is the same as the
 *                 configuration retrieved from the file's fapl.
 *              C) Verify the following when configured as VFD SWMR reader:
 *                  (1) there is an existing file opened as writer:
 *                      --same process open as reader: will just increment the
 *                        file reference count and use the same shared struct
 *                  (2) there is no existing file opened as writer:
 *                      --opening the file as reader will fail
 *                        because there is no metadata file
 *
 * Return:      0 if test is sucessful
 *              1 if test fails
 *
 * Programmer:  Vailin Choi; July 2018
 *
 *-------------------------------------------------------------------------
 */
static unsigned
test_file_fapl(void)
{
    hid_t fid = -1;         /* File ID */
    hid_t fid_read = -1;    /* File ID */
    hid_t fcpl = -1;        /* File creation property list ID */
    hid_t fapl1 = -1;       /* File access property list ID */
    hid_t fapl2 = -1;       /* File access property list ID */
    hid_t file_fapl = -1;   /* File access property list ID associated with the file */
    H5F_vfd_swmr_config_t *config1 = NULL;      /* Configuration for VFD SWMR */
    H5F_vfd_swmr_config_t *config2 = NULL;      /* Configuration for VFD SWMR */
    H5F_vfd_swmr_config_t *file_config = NULL;  /* Configuration for VFD SWMR */

    TESTING("VFD SWMR configuration for the file and fapl");

    /* Should succeed without VFD SWMR configured */
    if((fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        TEST_ERROR;

    /* Close the file  */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    /* Allocate memory for the configuration structure */
    if((config1 = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    if((config2 = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    if((file_config = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    HDmemset(config1, 0, sizeof(H5F_vfd_swmr_config_t));
    HDmemset(config2, 0, sizeof(H5F_vfd_swmr_config_t));
    HDmemset(file_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Create a copy of the file access property list */
    if((fapl1 = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        TEST_ERROR;

    /* Configured as VFD SWMR writer */
    config1->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    config1->tick_len = 4; 
    config1->max_lag = 6;
    config1->vfd_swmr_writer = TRUE;
    config1->md_pages_reserved = 2;
    HDstrcpy(config1->md_file_path, MD_FILENAME);

    /* Should succeed in setting the VFD SWMR configuration */
    if(H5Pset_vfd_swmr_config(fapl1, config1) < 0)
        TEST_ERROR;

    /* Should fail to create: page buffering and paged aggregation not enabled */
    H5E_BEGIN_TRY {
        fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, fapl1);
    } H5E_END_TRY;
    if(fid >= 0)
        TEST_ERROR;

    /* Create a copy of the file creation property list */
    if((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            FAIL_STACK_ERROR

    /* Set file space strategy */
    if(H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, FALSE, (hsize_t)1) < 0)
        FAIL_STACK_ERROR;

    /* Should fail to create: no page buffering */
    H5E_BEGIN_TRY {
        fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, fcpl, fapl1);
    } H5E_END_TRY;
    if(fid >= 0)
        TEST_ERROR;

    /* Enable page buffering */
    if(H5Pset_page_buffer_size(fapl1, 4096, 0, 0) < 0)
        FAIL_STACK_ERROR;

    /* Should succeed to create the file: paged aggregation and page buffering enabled */
    if((fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, fcpl, fapl1)) < 0)
        TEST_ERROR;

    /* Get the file's file access property list */
    if((file_fapl = H5Fget_access_plist(fid)) < 0)
        FAIL_STACK_ERROR;

    /* Retrieve the VFD SWMR configuration from file_fapl */
    if(H5Pget_vfd_swmr_config(file_fapl, file_config) < 0)
        TEST_ERROR;

    /* Verify the retrieved info is the same as config1 */
    if(HDmemcmp(config1, file_config, sizeof(H5F_vfd_swmr_config_t)) != 0)
        TEST_ERROR;

    /* Closing */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(file_fapl) < 0)
        FAIL_STACK_ERROR;


    /* Should succeed to open the file as VFD SWMR writer */
    if((fid = H5Fopen(FILENAME, H5F_ACC_RDWR, fapl1)) < 0)
        TEST_ERROR;

    /* Get the file's file access property list */
    if((file_fapl = H5Fget_access_plist(fid)) < 0)
        FAIL_STACK_ERROR;

    /* Clear info in file_config */
    HDmemset(file_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Retrieve the VFD SWMR configuration from file_fapl */
    if(H5Pget_vfd_swmr_config(file_fapl, file_config) < 0)
        TEST_ERROR;

    /* Verify the retrieved info is the same as config1 */
    if(HDmemcmp(config1, file_config, sizeof(H5F_vfd_swmr_config_t)) != 0)
        TEST_ERROR;

    /* Closing */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(file_fapl) < 0)
        FAIL_STACK_ERROR;

    /* Create a copy of the file access property list */
    if((fapl2 = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        TEST_ERROR;

    /* Set up different VFD SWMR configuration */
    config2->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    config2->tick_len = 4; 
    config2->max_lag = 10;
    config2->vfd_swmr_writer = TRUE;
    config2->md_pages_reserved = 2;
    HDstrcpy(config2->md_file_path, MD_FILENAME);

    /* Should succeed in setting the VFD SWMR configuration */
    if(H5Pset_vfd_swmr_config(fapl2, config2) < 0)
        TEST_ERROR;

    /* Enable page buffering */
    if(H5Pset_page_buffer_size(fapl2, 4096, 0, 0) < 0)
        FAIL_STACK_ERROR;

    /* Should succeed to open the file as VFD SWMR writer */
    if((fid = H5Fopen(FILENAME, H5F_ACC_RDWR, fapl2)) < 0)
        TEST_ERROR;

    /* Get the file's file access property list */
    if((file_fapl = H5Fget_access_plist(fid)) < 0)
        FAIL_STACK_ERROR;

    /* Clear info in file_config */
    HDmemset(file_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Retrieve the VFD SWMR configuration from file_fapl */
    if(H5Pget_vfd_swmr_config(file_fapl, file_config) < 0)
        TEST_ERROR;

    /* Verify the retrieved info is NOT the same as config1 */
    if(HDmemcmp(config1, file_config, sizeof(H5F_vfd_swmr_config_t)) == 0)
        TEST_ERROR;

    /* Verify the retrieved info is the same as config2 */
    if(HDmemcmp(config2, file_config, sizeof(H5F_vfd_swmr_config_t)) != 0)
        TEST_ERROR;

    /*
     * The file previously opened as writer is not closed.
     */
    /* Create a copy of the file access property list */
    if((fapl2 = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        TEST_ERROR;

    /* Set up as VFD SWMR reader */
    config2->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    config2->tick_len = 4; 
    config2->max_lag = 10;
    config2->vfd_swmr_writer = FALSE;
    config2->md_pages_reserved = 2;
    HDstrcpy(config2->md_file_path, MD_FILENAME);

    /* Should succeed in setting the VFD SWMR configuration in fapl2 */
    if(H5Pset_vfd_swmr_config(fapl2, config2) < 0)
        TEST_ERROR;

    /* Enable page buffering */
    if(H5Pset_page_buffer_size(fapl2, 4096, 0, 0) < 0)
        FAIL_STACK_ERROR;

    /* Should succeed in opening the file */
    /* Same process open: even though opened with reader configuration, 
     * it just increments the file reference count and uses the writer's 
     * shared file struct */
    if((fid_read = H5Fopen(FILENAME, H5F_ACC_RDONLY, fapl2)) < 0)
        TEST_ERROR;

    /* Clear info in file_config */
    HDmemset(file_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Get the file's file access property list */
    if((file_fapl = H5Fget_access_plist(fid)) < 0)
        FAIL_STACK_ERROR;

    /* Retrieve the VFD SWMR configuration from file_fapl */
    if(H5Pget_vfd_swmr_config(file_fapl, file_config) < 0)
        TEST_ERROR;

    /* Verify that the retrieved config is a writer */
    if(file_config->vfd_swmr_writer == FALSE)
        TEST_ERROR;
    /* Verify that the retrieved config is not the same as the initial configuration */
    if(file_config->vfd_swmr_writer == config2->vfd_swmr_writer)
        TEST_ERROR;

    /* Closing */
    if(H5Fclose(fid_read) < 0)
        FAIL_STACK_ERROR;
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(file_fapl) < 0)
        FAIL_STACK_ERROR;

    /*
     * The file opened as writer is closed.
     */
    /* Should fail to open the file as VFD SWMR reader: no metadata file */
    H5E_BEGIN_TRY {
        fid_read = H5Fopen(FILENAME, H5F_ACC_RDONLY, fapl2);
    } H5E_END_TRY;
    if(fid_read >= 0)
        TEST_ERROR;

    /* Closing */
    if(H5Pclose(fapl1) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(fapl2) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(fcpl) < 0)
        FAIL_STACK_ERROR;

    /* Free buffers */
    if(config1)
        HDfree(config1);
    if(config2)
        HDfree(config2);
    if(file_config)
        HDfree(file_config);

    PASSED()
    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl1);
        H5Pclose(fapl2);
        H5Pclose(fcpl);
        H5Fclose(fid);
    } H5E_END_TRY;
    if(config1)
        HDfree(config1);
    if(config2)
        HDfree(config2);
    if(file_config)
        HDfree(file_config);
    return 1;
} /* test_file_fapl() */


/*-------------------------------------------------------------------------
 * Function:    test_file_end_tick()
 *
 * Purpose:     Verify the public routine H5Fvfd_swmr_end_tick() works
 *              as described in the RFC for VFD SWMR.
 *              --routine will fail if the file is not opened with VFD SWMR
 *              ?? Will add more tests when end of tick processing 
 *                 is activated in this routine
 *
 * Return:      0 if test is sucessful
 *              1 if test fails
 *
 * Programmer:  Vailin Choi; July 2018
 *
 *-------------------------------------------------------------------------
 */
static unsigned
test_file_end_tick(void)
{
    hid_t fid = -1;     /* File ID */
    hid_t fapl = -1;    /* File access property list */
    hid_t fcpl = -1;    /* File creation property list */
    H5F_vfd_swmr_config_t *my_config = NULL;    /* Configuration for VFD SWMR */
    herr_t ret;         /* Return value */

    TESTING("H5Fvfd_swmr_end_tick() for VFD SWMR");

    /* Should succeed without VFD SWMR configured */
    if((fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        TEST_ERROR;

    /* Should fail */
    H5E_BEGIN_TRY {
        ret = H5Fvfd_swmr_end_tick(fid);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Close the file  */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    /* Allocate memory for the configuration structure */
    if((my_config = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    HDmemset(my_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Create a copy of the file access property list */
    if((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        FAIL_STACK_ERROR;
    
    /* Set the configuration */
    my_config->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    my_config->tick_len = 3; 
    my_config->max_lag = 3;
    my_config->vfd_swmr_writer = TRUE;
    my_config->md_pages_reserved = 2;
    HDstrcpy(my_config->md_file_path, MD_FILENAME);

    /* Should succeed in setting the VFD SWMR configuration */
    if(H5Pset_vfd_swmr_config(fapl, my_config) < 0)
        TEST_ERROR;

    /* Enable page buffering */
    if(H5Pset_page_buffer_size(fapl, 4096, 0, 0) < 0)
        FAIL_STACK_ERROR;

    /* Create a copy of file creation property list */
    if((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
        FAIL_STACK_ERROR

    /* Set file space strategy */
    if(H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, FALSE, (hsize_t)1) < 0)
        FAIL_STACK_ERROR;

    /* Create the file with VFD SWMR configured */
    if((fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, fcpl, fapl)) < 0)
        FAIL_STACK_ERROR;

    /* Should succeed */
    if(H5Fvfd_swmr_end_tick(fid) < 0)
        TEST_ERROR;

    /* Close the file */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    /* Open the file as VFD SWMR writer */
    if((fid = H5Fopen(FILENAME, H5F_ACC_RDWR, fapl)) < 0)
        TEST_ERROR;

    /* Should succeed */
    if(H5Fvfd_swmr_end_tick(fid) < 0)
        TEST_ERROR;

    /* Close the file */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    /* Open the file as reader without VFD SWMR configured */
    if((fid = H5Fopen(FILENAME, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        FAIL_STACK_ERROR;

    /* Should fail */
    H5E_BEGIN_TRY {
        ret = H5Fvfd_swmr_end_tick(fid);
    } H5E_END_TRY;
    if(ret >= 0)
        TEST_ERROR;

    /* Close the file */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    if(H5Pclose(fapl) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(fcpl) < 0)
        FAIL_STACK_ERROR;
    if(my_config)
        HDfree(my_config);

    PASSED()
    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl);
        H5Pclose(fcpl);
        H5Fclose(fid);
    } H5E_END_TRY;

    if(my_config)
        HDfree(my_config);

    return 1;
} /* test_file_end_tick() */


/*-------------------------------------------------------------------------
 * Function:    test_writer_md()
 *
 * Purpose:     Verify info in the metadata file when:
 *              --creating the HDF5 file
 *              --flushing the HDF5 file
 *              --opening an existing HDF5 file
 *              It will call the internal testing routine 
 *              H5F__vfd_swmr_writer_md_test() to do the following:
 *              --Open the metadata file
 *              --Verify the file size is as expected (md_pages_reserved)
 *              --For file create:
 *                  --No header magic is found
 *              --For file open or file flush:
 *                  --Read and decode the header and index in the metadata file
 *                  --Verify info in the header and index read from
 *                    the metadata file is as expected (empty index)
 *
 * Return:      0 if test is sucessful
 *              1 if test fails
 *
 * Programmer:  Vailin Choi; October 2018
 *
 *-------------------------------------------------------------------------
 */
static unsigned
test_writer_md(void)
{
    hid_t fid = -1;     /* File ID */
    hid_t fapl = -1;    /* File access property list */
    hid_t fcpl = -1;    /* File creation property list */
    H5F_vfd_swmr_config_t *my_config = NULL;    /* Configuration for VFD SWMR */

    TESTING("Create/Open/Flush an HDF5 file for VFD SWMR");

    /* Allocate memory for the configuration structure */
    if((my_config = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    HDmemset(my_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Create a copy of the file access property list */
    if((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        FAIL_STACK_ERROR;
    
    /* Set up the VFD SWMR configuration */
    my_config->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    my_config->tick_len = 1; 
    my_config->max_lag = 3;
    my_config->vfd_swmr_writer = TRUE;
    my_config->md_pages_reserved = 1;
    HDstrcpy(my_config->md_file_path, MD_FILENAME);

    /* Set the VFD SWMR configuration in fapl */
    if(H5Pset_vfd_swmr_config(fapl, my_config) < 0)
        FAIL_STACK_ERROR;

    /* Enable page buffering */
    if(H5Pset_page_buffer_size(fapl, 4096, 0, 0) < 0)
        FAIL_STACK_ERROR;

    /* Create a copy of the file creation property list */
    if((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
        FAIL_STACK_ERROR

    /* Set file space strategy */
    if(H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, FALSE, (hsize_t)1) < 0)
        FAIL_STACK_ERROR;

    /* Create an HDF5 file with VFD SWMR configured */
    if((fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, fcpl, fapl)) < 0)
        FAIL_STACK_ERROR;

    /* Verify info in metadata file when creating the HDF5 file */
    if(H5F__vfd_swmr_writer_md_test(fid, TRUE) < 0)
        TEST_ERROR

    /* Flush the HDF5 file */
    if(H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0) 
        FAIL_STACK_ERROR

    /* Verify info in metadata file when flushing the HDF5 file */
    if(H5F__vfd_swmr_writer_md_test(fid, FALSE) < 0)
        TEST_ERROR

    /* Close the file */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    /* Re-open the file as VFD SWMR writer */
    if((fid = H5Fopen(FILENAME, H5F_ACC_RDWR, fapl)) < 0)
        TEST_ERROR;

    /* Verify info in metadata file when reopening the HDF5 file */
    if(H5F__vfd_swmr_writer_md_test(fid, FALSE) < 0)
        TEST_ERROR

    /* Closing */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(fapl) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(fcpl) < 0)
        FAIL_STACK_ERROR;

    if(my_config)
        HDfree(my_config);

    PASSED()
    return 0;

error:
    if(my_config)
        HDfree(my_config);
    
    H5E_BEGIN_TRY {
        H5Pclose(fapl);
        H5Pclose(fcpl);
        H5Fclose(fid);
    } H5E_END_TRY;

    return 1;
} /* test_writer_md() */


/*-------------------------------------------------------------------------
 * Function:    test_writer_update_md()
 *
 * Purpose:     Verify info in the metadata file after update with the 
 *              constructed index: (A), (B), (C), (D)
 *              It will call the internal testing routine 
 *              H5F__vfd_swmr_writer_update_md_test() to do the following:
 *              --Update the metadata file with the input index via the
 *                internal library routine H5F_update_vfd_swmr_metadata_file()
 *              --Verify the entries in the delayed list is as expected
 *                (input: num_insert_dl, num_remove_dl)
 *              --Open the metadata file, read and decode the header and index
 *              --Verify info in the header and index just read from the
 *                metadatea file is as expected (input: num_entries and index)
 *
 * Return:      0 if test is sucessful
 *              1 if test fails
 *
 * Programmer:  Vailin Choi; October 2018
 *
 *-------------------------------------------------------------------------
 */
static unsigned
test_writer_update_md(void)
{
    hid_t fid = -1;             /* File ID */
    hid_t fid_read = -1;        /* File ID for the reader */
    hid_t fapl = -1;            /* File access property list */
    hid_t fapl2 = -1;           /* File access property list */
    hid_t fcpl = -1;            /* File creation property list */
    unsigned num_entries = 10;  /* Number of entries in the index */
    unsigned i = 0;             /* Local index variables */
    uint8_t *buf = NULL;        /* Data page from the page buffer */
    hid_t dcpl = -1;            /* Dataset creation property list */
    hid_t sid = -1;             /* Dataspace ID */
    hid_t did = -1;             /* Dataset ID */
    int *rwbuf = NULL;          /* Data buffer for writing */
    H5O_info_t oinfo;           /* Object metadata information */
    char dname[50];             /* Name of dataset */
    hsize_t dims[2] = {50, 20}; /* Dataset dimension sizes */
    hsize_t max_dims[2] = {H5S_UNLIMITED, H5S_UNLIMITED}; /* Dataset maximum dimension sizes */
    hsize_t chunk_dims[2] = {2, 5};             /* Dataset chunked dimension sizes */
    H5FD_vfd_swmr_idx_entry_t *index = NULL;    /* Pointer to the index entries */
    H5F_vfd_swmr_config_t *my_config = NULL;    /* Configuration for VFD SWMR */

    TESTING("Updating the metadata file for VFD SWMR writer");

    /* Allocate memory for the configuration structure */
    if((my_config = (H5F_vfd_swmr_config_t *)HDmalloc(sizeof(H5F_vfd_swmr_config_t))) == NULL)
        FAIL_STACK_ERROR;
    HDmemset(my_config, 0, sizeof(H5F_vfd_swmr_config_t));

    /* Create a copy of the file access property list */
    if((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        FAIL_STACK_ERROR;
    
    /* Set up the VFD SWMR configuration */
    my_config->version = H5F__CURR_VFD_SWMR_CONFIG_VERSION;
    my_config->tick_len = 1; 
    my_config->max_lag = 3;
    my_config->vfd_swmr_writer = TRUE;
    my_config->md_pages_reserved = 2;
    HDstrcpy(my_config->md_file_path, MD_FILENAME);

    /* Set the VFD SWMR configuration in fapl */
    if(H5Pset_vfd_swmr_config(fapl, my_config) < 0)
        FAIL_STACK_ERROR;

    /* Enable page buffering */
    if(H5Pset_page_buffer_size(fapl, FS_PAGE_SIZE, 0, 0) < 0)
        FAIL_STACK_ERROR;

    /* Create a copy of the file creation property list */
    if((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
        FAIL_STACK_ERROR

    /* Set file space strategy */
    if(H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, FALSE, (hsize_t)1) < 0)
        FAIL_STACK_ERROR;
    if(H5Pset_file_space_page_size(fcpl, FS_PAGE_SIZE) < 0)
        FAIL_STACK_ERROR;

    /* Create an HDF5 file with VFD SWMR configured */
    if((fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, fcpl, fapl)) < 0)
        FAIL_STACK_ERROR;

    /* Verify info in the metadata file when creating an HDF5 file */
    if(H5F__vfd_swmr_writer_md_test(fid, TRUE) < 0)
        TEST_ERROR

    /* Allocate num_entries for the data buffer */
    if((buf = (uint8_t *)HDmalloc((num_entries * FS_PAGE_SIZE * sizeof(uint8_t)))) == NULL)
        FAIL_STACK_ERROR;

    /* Allocate memory for num_entries index */
    if(NULL == (index = (H5FD_vfd_swmr_idx_entry_t *)HDcalloc(num_entries, sizeof(H5FD_vfd_swmr_idx_entry_t))))
        FAIL_STACK_ERROR;

    /* (A) Construct index for updating the metadata file */
    for(i = 0; i < num_entries; i++) {
        index[i].hdf5_page_offset = (uint64_t)my_config->md_pages_reserved;
        index[i].md_file_page_offset = 0;
        index[i].length = (uint32_t)FS_PAGE_SIZE;
        index[i].entry_ptr = (void *)&buf[i];
    }

    /* Update with index and verify info in the metadata file */
    /* Also verify that 0/0 entries are inserted/removed to/from the delayed list */
    if(H5F__vfd_swmr_writer_update_md_test(fid, num_entries, index, 0, 0) < 0)
        TEST_ERROR

    /* Create dataset creation property list */
    if((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        FAIL_STACK_ERROR

    /* Set to use chunked dataset */
    if(H5Pset_chunk(dcpl, 2, chunk_dims) < 0)
        FAIL_STACK_ERROR

    /* Create dataspace */
    if((sid = H5Screate_simple(2, dims, max_dims)) < 0)
        FAIL_STACK_ERROR

    /* Perform activities to ensure that max_lag ticks elapse */
    for(i = 0; i < 500; i++) {

        /* Create a chunked dataset */
        sprintf(dname, "dset %d", i);
        if((did = H5Dcreate2(fid, dname, H5T_NATIVE_INT, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
            FAIL_STACK_ERROR

        /* Get dataset object header address */
        if(H5Oget_info2(did, &oinfo, H5O_INFO_BASIC) < 0)
            FAIL_STACK_ERROR

        /* Close the dataset */
        if(H5Dclose(did) < 0)
            FAIL_STACK_ERROR
    }

    /* (B) Update every other entry in the index */
    for(i = 0; i < num_entries; i+= 2)
        index[i].entry_ptr = (void *)&buf[i];

    /* Update with index and verify info in the metadata file */
    /* Also verify that 5/0 entries are inserted/removed to/from the delayed list */
    if(H5F__vfd_swmr_writer_update_md_test(fid, num_entries, index, 5, 0) < 0)
        TEST_ERROR

    /* Allocate memory for the read/write buffer */
    if((rwbuf = (int *)HDmalloc(sizeof(int) * (50 * 20))) == NULL)
        FAIL_STACK_ERROR;
    for(i = 0; i < (50 * 20); i++)
        rwbuf[i] = (int)i;

    /* Perform activities to ensure that max_lag ticks elapse */
    for(i = 0; i < 500; i++) {
        /* Open the dataset */
        sprintf(dname, "dset %d", i);
        if((did = H5Dopen2(fid, dname, H5P_DEFAULT)) < 0)
            FAIL_STACK_ERROR

        /* Write to the dataset */
        if(H5Dwrite(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, rwbuf) < 0)
            FAIL_STACK_ERROR

        /* Get dataset object info */
        if(H5Oget_info2(did, &oinfo, H5O_INFO_BASIC) < 0)
            FAIL_STACK_ERROR

        /* Close the dataset */
        if(H5Dclose(did) < 0)
            FAIL_STACK_ERROR
    }

    /* (C) Update every 3 entry in the index */
    for(i = 0; i < num_entries; i+= 3)
        index[i].entry_ptr = (void *)&buf[i];

    /* Update with index and verify info in the metadata file */
    /* Also verify that 4/5 entries are inserted/removed to/from the delayed list */
    if(H5F__vfd_swmr_writer_update_md_test(fid, num_entries, index, 4, 5) < 0)
        TEST_ERROR

    /* Clear the read/write buffer */
    HDmemset(rwbuf, 0, sizeof(sizeof(int) * (50 * 20)));

    /* Perform activities to ensure that max_lag ticks elapse */
    for(i = 0; i < 500; i++) {
        /* Open the dataset */
        sprintf(dname, "dset %d", i);
        if((did = H5Dopen2(fid, dname, H5P_DEFAULT)) < 0)
            FAIL_STACK_ERROR

        /* Read from the dataset */
        if(H5Dread(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, rwbuf) < 0)
            FAIL_STACK_ERROR

        /* Get dataset object info */
        if(H5Oget_info2(did, &oinfo, H5O_INFO_BASIC) < 0)
            FAIL_STACK_ERROR

        /* Close the dataset */
        if(H5Dclose(did) < 0)
            FAIL_STACK_ERROR
    }

    /* (D) Update two entries in the index */
    index[1].entry_ptr = (void *)&buf[1];
    index[5].entry_ptr = (void *)&buf[5];

    /* Update with index and verify info in the metadata file */
    /* Also verify that 2/4 entries are inserted/removed to/from the delayed list */
    if(H5F__vfd_swmr_writer_update_md_test(fid, num_entries, index, 2, 4) < 0)
        TEST_ERROR

    /* Close the file */
    if(H5Fclose(fid) < 0)
        FAIL_STACK_ERROR;

    if(H5Sclose(sid) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(dcpl) < 0)
        FAIL_STACK_ERROR;

    if(H5Pclose(fapl) < 0)
        FAIL_STACK_ERROR;
    if(H5Pclose(fcpl) < 0)
        FAIL_STACK_ERROR;

    if(my_config)
        HDfree(my_config);

    if(buf)
        HDfree(buf);
    if(rwbuf)
        HDfree(rwbuf);

    PASSED()
    return 0;

error:
    if(my_config)
        HDfree(my_config);
     if(buf)
        HDfree(buf);
    if(rwbuf)
        HDfree(rwbuf);
    if(index)
        HDfree(index);

    H5E_BEGIN_TRY {
        H5Dclose(did);
        H5Sclose(sid);
        H5Pclose(dcpl);
        H5Pclose(fapl);
        H5Pclose(fcpl);
        H5Fclose(fid);
    } H5E_END_TRY;

    return 1;
} /* test_writer_update_md() */


/*-------------------------------------------------------------------------
 * Function:    main()
 *
 * Purpose:     Main function for VFD SWMR tests.
 *
 * Return:      0 if test is sucessful
 *              1 if test fails
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    hid_t       fapl = -1;              /* File access property list for data files */
    unsigned    nerrors = 0;            /* Cumulative error count */
    const char  *env_h5_drvr = NULL;    /* File Driver value from environment */
    hbool_t     api_ctx_pushed = FALSE;             /* Whether API context pushed */

    h5_reset();

    /* Get the VFD to use */
    env_h5_drvr = HDgetenv("HDF5_DRIVER");
    if(env_h5_drvr == NULL)
        env_h5_drvr = "nomatch";

    /* Temporary skip testing with multi/split drivers:
     * Page buffering depends on paged aggregation which is
     * currently disabled for multi/split drivers.
     */
    if((0 == HDstrcmp(env_h5_drvr, "multi")) || 
       (0 == HDstrcmp(env_h5_drvr, "split"))) {

        SKIPPED()
        HDputs("Skip VFD SWMR test because paged aggregation is disabled for multi/split drivers");
        HDexit(EXIT_SUCCESS);
    } /* end if */

    if((fapl = h5_fileaccess()) < 0) {
        nerrors++;
        PUTS_ERROR("Can't get VFD-dependent fapl")
    } /* end if */

    /* Push API context */
    if(H5CX_push() < 0) FAIL_STACK_ERROR
        api_ctx_pushed = TRUE;

    nerrors += test_fapl();
    nerrors += test_file_fapl();
    nerrors += test_file_end_tick();

    nerrors += test_writer_md();
    nerrors += test_writer_update_md();

    if(nerrors)
        goto error;

    /* Pop API context */
    if(api_ctx_pushed && H5CX_pop() < 0) FAIL_STACK_ERROR
        api_ctx_pushed = FALSE;

    HDputs("All VFD SWMR tests passed.");

    HDexit(EXIT_SUCCESS);

error:
    HDprintf("***** %d VFD SWMR TEST%s FAILED! *****\n",
        nerrors, nerrors > 1 ? "S" : "");

    H5E_BEGIN_TRY {
        H5Pclose(fapl);
    } H5E_END_TRY;

    if(api_ctx_pushed) H5CX_pop();

    HDexit(EXIT_FAILURE);
} /* main() */

