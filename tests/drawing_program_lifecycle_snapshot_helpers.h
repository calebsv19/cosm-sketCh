#ifndef DRAWING_PROGRAM_LIFECYCLE_SNAPSHOT_HELPERS_H
#define DRAWING_PROGRAM_LIFECYCLE_SNAPSHOT_HELPERS_H

int write_legacy_snapshot_without_layer_chunk(const char *source_pack_path,
                                              const char *output_pack_path);
int write_legacy_snapshot_without_object_chunk(const char *source_pack_path,
                                               const char *output_pack_path);
int write_legacy_snapshot_with_v1_object_chunk(const char *source_pack_path,
                                               const char *output_pack_path);
int write_legacy_snapshot_with_v2_object_chunk(const char *source_pack_path,
                                               const char *output_pack_path);
int write_snapshot_with_invalid_path_payload(const char *source_pack_path,
                                             const char *output_pack_path);

#endif
