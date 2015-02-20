include Makefile.builtem

include ../src/Makefile.include

TARGETS = compile_test json_builder_test fileserver libmicrohttpd_fileserver xml_builder_test

C_FLAGS += $(call include_dir,../src)
LD_FLAGS += $(call use_library,$(if $(filter win-%,$(PLATFORM)),$(MICRORESTD_LIBRARIES_WIN),$(MICRORESTD_LIBRARIES_POSIX)))

.PHONY: all compile
all: compile

compile: $(call exe,$(TARGETS))

$(call exe,compile_test): $(call obj,$(MICRORESTD_PUGIXML_OBJECTS:%=../src/%))
$(call exe,$(TARGETS)): $(call exe,%): $$(call obj,%) $(call obj,$(MICRORESTD_OBJECTS:%=../src/%))
	$(call link_exe,$@,$^,$(call win_subsystem,console))

.PHONY: clean
clean:
	$(call rm,.build $(call all_exe,$(TARGETS)))
