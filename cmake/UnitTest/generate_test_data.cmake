# Copy files from source directory to destination directory, substituting any
# variables.  Create destination directory if it does not exist.

function(configure_files srcDir destDir)
	make_directory(${destDir})

	file(GLOB templateFiles RELATIVE ${srcDir} ${srcDir}/*)
	foreach(templateFile ${templateFiles})
		set(srcTemplatePath ${srcDir}/${templateFile})
		if(NOT IS_DIRECTORY ${srcTemplatePath})
			configure_file(
					${srcTemplatePath}
					${destDir}/${templateFile}
					@ONLY
					NEWLINE_STYLE LF
			)
		else()
			configure_files("${srcTemplatePath}" "${destDir}/${templateFile}")
		endif()
	endforeach()
endfunction()

configure_files(${SOURCE} ${DESTINATION})