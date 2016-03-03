file(GLOB data_files "${MultiMC_TEST_DATA_PATH_SOURCE}/*")
foreach(data_file ${data_files})
	get_filename_component(filename ${data_file} NAME)
	configure_file(
		${data_file}
		${MultiMC_TEST_DATA_PATH}/${filename}
		@ONLY
		NEWLINE_STYLE LF
	)
endforeach()

file(GLOB raw_data_files "${MultiMC_TEST_DATA_PATH_SOURCE_RAW}/*")
file(COPY ${raw_data_files} DESTINATION ${MultiMC_TEST_DATA_PATH}/)
