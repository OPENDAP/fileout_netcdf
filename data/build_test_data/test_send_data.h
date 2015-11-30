/*
 * test_send_data.h
 *
 *  Created on: Feb 3, 2013
 *      Author: jimg
 */

#ifndef TEST_SEND_DATA_H_
#define TEST_SEND_DATA_H_

void build_dods_response(DataDDS* dds, const string &file_name);
void build_netcdf_file(DataDDS* dds, const string &file_name);
void send_data(DataDDS *dds, ConstraintEvaluator &eval, BESDataHandlerInterface &dhi);

#endif /* TEST_SEND_DATA_H_ */
