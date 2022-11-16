# Makefile to build class 'flite' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = flite

cflags =
w32_cflags =

BUNDLED_FLITE = ./deps/flite
ifneq ($(wildcard $(BUNDLED_FLITE)/include/flite.h),)
  use_bundled_flite=yes
endif

ifeq ($(use_bundled_flite), yes)
cflags += \
	-I$(BUNDLED_FLITE)/include \
	-I$(BUNDLED_FLITE)/lang/cmulex \
	-I$(BUNDLED_FLITE)/lang/usenglish \
	$(empty)

# urgh, this is ugly!
# we cannot really statically link against flite on Windows,
# as some global variables are declared with '__declspec(dllexport)'.
# so we just disable the entire __declspec() magic here:
w32_cflags += \
	-D"__declspec(x)=" \
	$(empty)

# a much better approach is to just build a static version of flite,
# but this requires https://github.com/festvox/flite/pull/84
# to be merged first:
w32_cflags += \
	-DFLITE_STATIC=1 \
	$(empty)

else
cflags += -DHAVE_FLITE_FLITE_H=1
ldlibs += \
	-lflite_cmulex \
	-lflite_cmu_grapheme_lang \
	-lflite_cmu_grapheme_lex \
	-lflite_cmu_indic_lang \
	-lflite_cmu_indic_lex \
	-lflite_cmu_time_awb \
	-lflite_cmu_us_awb \
	-lflite_cmu_us_kal \
	-lflite_cmu_us_kal16 \
	-lflite_cmu_us_rms \
	-lflite_cmu_us_slt \
	-lflite_usenglish \
	-lflite \
	$(empty)
endif


ldlibs += -lm -lpthread

cflags += -I . -DVERSION='"0.3.3"'

# input source file (class name == source file basename)
flite.class.sources = flite.c

ifeq ($(use_bundled_flite), yes)
flite.class.sources += \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex_data.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex_entries.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lts_model.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lts_rules.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_postlex.c \
	 $(BUNDLED_FLITE)/lang/cmu_grapheme_lang/cmu_grapheme_lang.c \
	 $(BUNDLED_FLITE)/lang/cmu_grapheme_lang/cmu_grapheme_phoneset.c \
	 $(BUNDLED_FLITE)/lang/cmu_grapheme_lang/cmu_grapheme_phrasing_cart.c \
	 $(BUNDLED_FLITE)/lang/cmu_grapheme_lang/graph_gpos.c \
	 $(BUNDLED_FLITE)/lang/cmu_grapheme_lex/cmu_grapheme_lex.c \
	 $(BUNDLED_FLITE)/lang/cmu_grapheme_lex/grapheme_unitran_tables.c \
	 $(BUNDLED_FLITE)/lang/cmu_indic_lang/cmu_indic_lang.c \
	 $(BUNDLED_FLITE)/lang/cmu_indic_lang/cmu_indic_phoneset.c \
	 $(BUNDLED_FLITE)/lang/cmu_indic_lang/cmu_indic_phrasing_cart.c \
	 $(BUNDLED_FLITE)/lang/cmu_indic_lex/cmu_indic_lex.c \
	 $(BUNDLED_FLITE)/lang/cmu_time_awb/cmu_time_awb.c \
	 $(BUNDLED_FLITE)/lang/cmu_time_awb/cmu_time_awb_cart.c \
	 $(BUNDLED_FLITE)/lang/cmu_time_awb/cmu_time_awb_clunits.c \
	 $(BUNDLED_FLITE)/lang/cmu_time_awb/cmu_time_awb_lex_entry.c \
	 $(BUNDLED_FLITE)/lang/cmu_time_awb/cmu_time_awb_lpc.c \
	 $(BUNDLED_FLITE)/lang/cmu_time_awb/cmu_time_awb_mcep.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_cg.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_cg_durmodel.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_cg_f0_trees.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_cg_phonestate.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_cg_single_mcep_trees.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_cg_single_params.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_spamf0_accent.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_spamf0_accent_params.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_awb/cmu_us_awb_spamf0_phrase.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal/cmu_us_kal.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal/cmu_us_kal_diphone.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal/cmu_us_kal_lpc.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal/cmu_us_kal_res.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal/cmu_us_kal_residx.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal/cmu_us_kal_ressize.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal16/cmu_us_kal16.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal16/cmu_us_kal16_diphone.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal16/cmu_us_kal16_lpc.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal16/cmu_us_kal16_res.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_kal16/cmu_us_kal16_residx.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_cg.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_cg_durmodel.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_cg_f0_trees.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_cg_phonestate.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_cg_single_mcep_trees.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_cg_single_params.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_spamf0_accent.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_spamf0_accent_params.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_rms/cmu_us_rms_spamf0_phrase.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_cg.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_cg_durmodel.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_cg_f0_trees.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_cg_phonestate.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_cg_single_mcep_trees.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_cg_single_params.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_spamf0_accent.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_spamf0_accent_params.c \
	 $(BUNDLED_FLITE)/lang/cmu_us_slt/cmu_us_slt_spamf0_phrase.c \
	 $(BUNDLED_FLITE)/lang/usenglish/usenglish.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_aswd.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_durz_cart.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_dur_stats.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_expand.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_f0lr.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_f0_model.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_ffeatures.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_gpos.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_int_accent_cart.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_int_tone_cart.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_nums_cart.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_phoneset.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_phrasing_cart.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_pos_cart.c \
	 $(BUNDLED_FLITE)/lang/usenglish/us_text.c \
	 $(BUNDLED_FLITE)/src/audio/au_none.c \
	 $(BUNDLED_FLITE)/src/audio/audio.c \
	 $(BUNDLED_FLITE)/src/audio/au_streaming.c \
	 $(BUNDLED_FLITE)/src/cg/cst_cg.c \
	 $(BUNDLED_FLITE)/src/cg/cst_cg_dump_voice.c \
	 $(BUNDLED_FLITE)/src/cg/cst_cg_load_voice.c \
	 $(BUNDLED_FLITE)/src/cg/cst_cg_map.c \
	 $(BUNDLED_FLITE)/src/cg/cst_mlpg.c \
	 $(BUNDLED_FLITE)/src/cg/cst_mlsa.c \
	 $(BUNDLED_FLITE)/src/cg/cst_spamf0.c \
	 $(BUNDLED_FLITE)/src/cg/cst_vc.c \
	 $(BUNDLED_FLITE)/src/hrg/cst_ffeature.c \
	 $(BUNDLED_FLITE)/src/hrg/cst_item.c \
	 $(BUNDLED_FLITE)/src/hrg/cst_relation.c \
	 $(BUNDLED_FLITE)/src/hrg/cst_rel_io.c \
	 $(BUNDLED_FLITE)/src/hrg/cst_utterance.c \
	 $(BUNDLED_FLITE)/src/lexicon/cst_lexicon.c \
	 $(BUNDLED_FLITE)/src/lexicon/cst_lts.c \
	 $(BUNDLED_FLITE)/src/lexicon/cst_lts_rewrites.c \
	 $(BUNDLED_FLITE)/src/regex/cst_regex.c \
	 $(BUNDLED_FLITE)/src/regex/regexp.c \
	 $(BUNDLED_FLITE)/src/regex/regsub.c \
	 $(BUNDLED_FLITE)/src/speech/cst_lpcres.c \
	 $(BUNDLED_FLITE)/src/speech/cst_track.c \
	 $(BUNDLED_FLITE)/src/speech/cst_track_io.c \
	 $(BUNDLED_FLITE)/src/speech/cst_wave.c \
	 $(BUNDLED_FLITE)/src/speech/cst_wave_io.c \
	 $(BUNDLED_FLITE)/src/speech/cst_wave_utils.c \
	 $(BUNDLED_FLITE)/src/speech/g721.c \
	 $(BUNDLED_FLITE)/src/speech/g723_24.c \
	 $(BUNDLED_FLITE)/src/speech/g723_40.c \
	 $(BUNDLED_FLITE)/src/speech/g72x.c \
	 $(BUNDLED_FLITE)/src/speech/rateconv.c \
	 $(BUNDLED_FLITE)/src/stats/cst_cart.c \
	 $(BUNDLED_FLITE)/src/stats/cst_ss.c \
	 $(BUNDLED_FLITE)/src/stats/cst_viterbi.c \
	 $(BUNDLED_FLITE)/src/synth/cst_ffeatures.c \
	 $(BUNDLED_FLITE)/src/synth/cst_phoneset.c \
	 $(BUNDLED_FLITE)/src/synth/cst_ssml.c \
	 $(BUNDLED_FLITE)/src/synth/cst_synth.c \
	 $(BUNDLED_FLITE)/src/synth/cst_utt_utils.c \
	 $(BUNDLED_FLITE)/src/synth/cst_voice.c \
	 $(BUNDLED_FLITE)/src/synth/flite.c \
	 $(BUNDLED_FLITE)/src/utils/cst_alloc.c \
	 $(BUNDLED_FLITE)/src/utils/cst_args.c \
	 $(BUNDLED_FLITE)/src/utils/cst_endian.c \
	 $(BUNDLED_FLITE)/src/utils/cst_error.c \
	 $(BUNDLED_FLITE)/src/utils/cst_features.c \
	 $(BUNDLED_FLITE)/src/utils/cst_file_stdio.c \
	 $(BUNDLED_FLITE)/src/utils/cst_mmap_none.c \
	 $(BUNDLED_FLITE)/src/utils/cst_socket.c \
	 $(BUNDLED_FLITE)/src/utils/cst_string.c \
	 $(BUNDLED_FLITE)/src/utils/cst_tokenstream.c \
	 $(BUNDLED_FLITE)/src/utils/cst_url.c \
	 $(BUNDLED_FLITE)/src/utils/cst_val.c \
	 $(BUNDLED_FLITE)/src/utils/cst_val_const.c \
	 $(BUNDLED_FLITE)/src/utils/cst_val_user.c \
	 $(BUNDLED_FLITE)/src/utils/cst_wchar.c \
	 $(BUNDLED_FLITE)/src/wavesynth/cst_clunits.c \
	 $(BUNDLED_FLITE)/src/wavesynth/cst_diphone.c \
	 $(BUNDLED_FLITE)/src/wavesynth/cst_reflpc.c \
	 $(BUNDLED_FLITE)/src/wavesynth/cst_sigpr.c \
	 $(BUNDLED_FLITE)/src/wavesynth/cst_sts.c \
	 $(BUNDLED_FLITE)/src/wavesynth/cst_units.c \
	 $(empty)

# unused sources
EXCLUDEDFILES = \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex_data_raw.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex_entries_huff_table.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex_num_bytes.c \
	 $(BUNDLED_FLITE)/lang/cmulex/cmu_lex_phones_huff_table.c \
	 $(BUNDLED_FLITE)/sapi/FliteTTSEngineObj/flite_sapi_usenglish.c \
	 $(BUNDLED_FLITE)/src/audio/auclient.c \
	 $(BUNDLED_FLITE)/src/audio/auserver.c \
	 $(BUNDLED_FLITE)/src/audio/au_alsa.c \
	 $(BUNDLED_FLITE)/src/audio/au_command.c \
	 $(BUNDLED_FLITE)/src/audio/au_oss.c \
	 $(BUNDLED_FLITE)/src/audio/au_palmos.c \
	 $(BUNDLED_FLITE)/src/audio/au_pulseaudio.c \
	 $(BUNDLED_FLITE)/src/audio/au_sun.c \
	 $(BUNDLED_FLITE)/src/audio/au_win.c \
	 $(BUNDLED_FLITE)/src/audio/au_wince.c \
	 $(BUNDLED_FLITE)/src/utils/cst_file_palmos.c \
	 $(BUNDLED_FLITE)/src/utils/cst_file_wince.c \
	 $(BUNDLED_FLITE)/src/utils/cst_mmap_posix.c \
	 $(BUNDLED_FLITE)/src/utils/cst_mmap_win32.c \
	 $(BUNDLED_FLITE)/testsuite/asciiS2U_main.c \
	 $(BUNDLED_FLITE)/testsuite/asciiU2S_main.c \
	 $(BUNDLED_FLITE)/testsuite/bin2ascii_main.c \
	 $(BUNDLED_FLITE)/testsuite/by_word_main.c \
	 $(BUNDLED_FLITE)/testsuite/combine_waves_main.c \
	 $(BUNDLED_FLITE)/testsuite/compare_wave_main.c \
	 $(BUNDLED_FLITE)/testsuite/dcoffset_wave_main.c \
	 $(BUNDLED_FLITE)/testsuite/flite_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/hrg_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/kal_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/lex_lookup_main.c \
	 $(BUNDLED_FLITE)/testsuite/lex_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/lpc_resynth_main.c \
	 $(BUNDLED_FLITE)/testsuite/lpc_test2_main.c \
	 $(BUNDLED_FLITE)/testsuite/lpc_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/multi_thread_main.c \
	 $(BUNDLED_FLITE)/testsuite/nums_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/play_client_main.c \
	 $(BUNDLED_FLITE)/testsuite/play_server_main.c \
	 $(BUNDLED_FLITE)/testsuite/play_sync_main.c \
	 $(BUNDLED_FLITE)/testsuite/play_wave_main.c \
	 $(BUNDLED_FLITE)/testsuite/record_in_noise_main.c \
	 $(BUNDLED_FLITE)/testsuite/record_wave_main.c \
	 $(BUNDLED_FLITE)/testsuite/regex_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/rfc_main.c \
	 $(BUNDLED_FLITE)/testsuite/token_test_main.c \
	 $(BUNDLED_FLITE)/testsuite/tris1_main.c \
	 $(BUNDLED_FLITE)/testsuite/utt_test_main.c \
	 $(BUNDLED_FLITE)/tools/find_sts_main.c \
	 $(BUNDLED_FLITE)/tools/flite_sort_main.c \
	 $(BUNDLED_FLITE)/tools/LANGNAME_lang.c \
	 $(BUNDLED_FLITE)/tools/LANGNAME_lex.c \
	 $(BUNDLED_FLITE)/tools/VOICE_cg.c \
	 $(BUNDLED_FLITE)/tools/VOICE_clunits.c \
	 $(BUNDLED_FLITE)/tools/VOICE_diphone.c \
	 $(BUNDLED_FLITE)/tools/VOICE_ldom.c \
	 $(BUNDLED_FLITE)/wince/flowm_flite.c \
	 $(BUNDLED_FLITE)/wince/flowm_main.c \
	 $(BUNDLED_FLITE)/main/compile_regexes.c \
	 $(BUNDLED_FLITE)/main/flitevox_info_main.c \
	 $(BUNDLED_FLITE)/main/flite_lang_list.c \
	 $(BUNDLED_FLITE)/main/flite_main.c \
	 $(BUNDLED_FLITE)/main/flite_time_main.c \
	 $(BUNDLED_FLITE)/main/t2p_main.c \
	 $(BUNDLED_FLITE)/main/word_times_main.c \
	 $(empty)
endif

define forWindows
  cflags += \
	-DCST_NO_SOCKETS \
	-DUNDER_WINDOWS \
	$(w32_cflags) \
	$(empty)
  ldlibs +=
endef

define forDarwin
  cflags += \
	-DCST_AUDIO_NONE \
	-no-cpp-precomp \
	$(empty)
endef

# all extra files to be included in binary distribution of the library
datafiles = \
	flite-help.pd \
	flite-numbers.pd flite-test2.pd flite-test.pd \
	README.md flite-meta.pd CHANGELOG.txt \


#alldebug: CPPFLAGS+=-DFLITE_DEBUG=1


# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

localdep_windows: install
	scripts/localdeps.win.sh "$(installpath)/flite.$(extension)"
