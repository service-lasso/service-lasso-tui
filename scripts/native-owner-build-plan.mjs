import { WINDOWS_NATIVE_ROOT_EXTENT } from './windows-native-root-layout.mjs';
import path from 'node:path';

// Closed production source roster. Source-only; no build/admission has run.
const roster = {
  "schemaVersion": 1,
  "linuxOwner": [
    "native/owner/archive_access_linux.c",
    "native/owner/archive_close_linux.c",
    "native/owner/archive_context_linux.c",
    "native/owner/archive_creation_linux.c",
    "native/owner/archive_directory_linux.c",
    "native/owner/archive_files_linux.c",
    "native/owner/archive_label_linux.c",
    "native/owner/archive_precreation_linux.c",
    "native/owner/archive_readback_linux.c",
    "native/owner/archive_reader_linux.c",
    "native/owner/bound_domains_linux.c",
    "native/owner/candidate_admission_linux.c",
    "native/owner/candidate_arena_linux.c",
    "native/owner/candidate_entry_linux.c",
    "native/owner/candidate_exec_context_linux.c",
    "native/owner/candidate_file_entry_linux.c",
    "native/owner/candidate_payload_context_linux.c",
    "native/owner/candidate_policy_linux.c",
    "native/owner/candidate_preexec_inspection_linux.c",
    "native/owner/candidate_preexec_linux.c",
    "native/owner/candidate_profile.c",
    "native/owner/candidate_routes_linux.c",
    "native/owner/candidate_socket_context_linux.c",
    "native/owner/candidate_socket_create_linux.c",
    "native/owner/candidate_startup_entry_linux.c",
    "native/owner/candidate_startup_file_entry_linux.c",
    "native/owner/candidate_startup_owner_entry_linux.c",
    "native/owner/candidate_startup_preexec_linux.c",
    "native/owner/candidate_startup_roles_linux.c",
    "native/owner/candidate_startup_routes_linux.c",
    "native/owner/candidate_startup_socket_context_linux.c",
    "native/owner/candidate_startup_socket_create_linux.c",
    "native/owner/candidate_startup_socket_use_linux.c",
    "native/owner/candidate_target_close_linux.c",
    "native/owner/candidate_target_linux.c",
    "native/owner/capability_domain_linux.c",
    "native/owner/capture_linux.c",
    "native/owner/capture_workers_linux.c",
    "native/owner/deadline_linux.c",
    "native/owner/deadline_poll_linux.c",
    "native/owner/cgroup_children_linux.c",
    "native/owner/cgroup_control_close_linux.c",
    "native/owner/cgroup_control_label_linux.c",
    "native/owner/collector_linux.c",
    "native/owner/context_reachability_linux.c",
    "native/owner/containing_charge_linux.c",
    "native/owner/creator_file_linux.c",
    "native/owner/public_inputs_linux.c",
    "native/owner/creator_elf_linux.c",
    "native/owner/control_access_linux.c",
    "native/owner/control_roster_linux.c",
    "native/owner/controller_linux.c",
    "native/owner/credentials_linux.c",
    "native/owner/custody_linux.c",
    "native/owner/domain_reachability.c",
    "native/owner/exec_label_linux.c",
    "native/owner/family_events_linux.c",
    "native/owner/final_capture_linux.c",
    "native/owner/forward_workers_linux.c",
    "native/owner/gate_linux.c",
    "native/owner/genfs_correspondence_linux.c",
    "native/owner/immutable_payload_linux.c",
    "native/owner/input_workers_linux.c",
    "native/owner/local_cgroup_linux.c",
    "native/owner/local_installation_linux.c",
    "native/owner/local_membership_linux.c",
    "native/owner/local_mounts_linux.c",
    "native/owner/local_payload_linux.c",
    "native/owner/lsm_self_linux.c",
    "native/owner/migration_labels_linux.c",
    "native/owner/migration_routes_linux.c",
    "native/owner/mount_location_linux.c",
    "native/owner/namespace_close_linux.c",
    "native/owner/namespace_parent_linux.c",
    "native/owner/namespace_target_linux.c",
    "native/owner/normal_capture_linux.c",
    "native/owner/owned_sha256.c",
    "native/owner/owner_channel_linux.c",
    "native/owner/owner_linux_entry.c",
    "native/owner/owner_linux.c",
    "native/owner/outer_birth_linux.c",
    "native/owner/outer_continuation_linux.c",
    "native/owner/outer_supervisor_linux.c",
    "native/owner/outer_terminal_linux.c",
    "native/owner/shared_range_archive_linux.c",
    "native/owner/failed_archive_linux.c",
    "native/owner/failed_archive_name_linux.c",
    "native/owner/deadline_fd_lift_linux.c",
    "native/owner/failed_parent_identity_linux.c",
    "native/owner/archive_declaration_linux.c",
    "native/owner/failed_parent_directory_linux.c",
    "native/owner/failed_parent_file_linux.c",
    "native/owner/parent_views_linux.c",
    "native/owner/pidfd_proc_linux.c",
    "native/owner/policy_parse_linux.c",
    "native/owner/preexec_authority_linux.c",
    "native/owner/preexec_inspection_linux.c",
    "native/owner/private_arguments.c",
    "native/owner/public_controller_linux.c",
    "native/owner/public_main_linux.c",
    "native/owner/public_startup_linux.c",
    "native/owner/schedule_linux.c",
    "native/owner/selinux_context_linux.c",
    "native/owner/selinux_policy_relations.c",
    "native/owner/selinux_snapshot_linux.c",
    "native/owner/selinux_status_linux.c",
    "native/owner/sepol_diagnostic_owner.c",
    "native/owner/sepol_parse_owner.c",
    "native/owner/sepol_source_owner.c",
    "native/owner/service_linux.c",
    "native/owner/signal_linux.c",
    "native/owner/socket_peer_linux.c",
    "native/owner/socket_state_linux.c",
    "native/owner/stream_routes_linux.c",
    "native/owner/terminal_linux.c"
  ],
  "windowsOwner": [
    "native/owner/argument_sender_windows.c",
    "native/owner/argument_sender_factory_windows.c",
    "native/owner/argument_root_windows.c",
    "native/owner/argument_reader_cancel_windows.c",
    "native/owner/channels_windows.c",
    "native/owner/debug_root_windows.c",
    "native/owner/entry_auth_windows.c",
    "native/owner/duplex_windows.c",
    "native/owner/candidate_profile.c",
    "native/owner/controller_windows.c",
    "native/owner/object_security_windows.c",
    "native/owner/owned_sha256.c",
    "native/owner/owner_windows_entry.c",
    "native/owner/owner_windows.c",
    "native/owner/private_arguments_windows.c",
    "native/owner/public_startup_windows.c",
    "native/owner/private_arguments.c",
    "native/owner/token_windows.c"
  ],
  "linuxSepol": [
    "third_party/libsepol/src/assertion.c",
    "third_party/libsepol/src/avrule_block.c",
    "third_party/libsepol/src/avtab.c",
    "third_party/libsepol/src/boolean_record.c",
    "third_party/libsepol/src/booleans.c",
    "third_party/libsepol/src/conditional.c",
    "third_party/libsepol/src/constraint.c",
    "third_party/libsepol/src/context_record.c",
    "third_party/libsepol/src/context.c",
    "third_party/libsepol/src/debug.c",
    "third_party/libsepol/src/ebitmap.c",
    "third_party/libsepol/src/expand.c",
    "third_party/libsepol/src/handle.c",
    "third_party/libsepol/src/hashtab.c",
    "third_party/libsepol/src/hierarchy.c",
    "third_party/libsepol/src/ibendport_record.c",
    "third_party/libsepol/src/ibendports.c",
    "third_party/libsepol/src/ibpkey_record.c",
    "third_party/libsepol/src/ibpkeys.c",
    "third_party/libsepol/src/iface_record.c",
    "third_party/libsepol/src/interfaces.c",
    "third_party/libsepol/src/kernel_to_cil.c",
    "third_party/libsepol/src/kernel_to_common.c",
    "third_party/libsepol/src/kernel_to_conf.c",
    "third_party/libsepol/src/link.c",
    "third_party/libsepol/src/mls.c",
    "third_party/libsepol/src/module_to_cil.c",
    "third_party/libsepol/src/module.c",
    "third_party/libsepol/src/node_record.c",
    "third_party/libsepol/src/nodes.c",
    "third_party/libsepol/src/optimize.c",
    "third_party/libsepol/src/polcaps.c",
    "third_party/libsepol/src/policydb_convert.c",
    "third_party/libsepol/src/policydb_public.c",
    "third_party/libsepol/src/policydb_validate.c",
    "third_party/libsepol/src/policydb.c",
    "third_party/libsepol/src/port_record.c",
    "third_party/libsepol/src/ports.c",
    "third_party/libsepol/src/services.c",
    "third_party/libsepol/src/sidtab.c",
    "third_party/libsepol/src/symtab.c",
    "third_party/libsepol/src/user_record.c",
    "third_party/libsepol/src/users.c",
    "third_party/libsepol/src/util.c",
    "third_party/libsepol/src/write.c"
  ]
};
for (const value of Object.values(roster)) if (Array.isArray(value)) Object.freeze(value);
export const nativeOwnerSourceRoster = Object.freeze(roster);

export function windowsRootOverlayMapping(goroot, sourceRoot) {
  if (typeof goroot !== 'string' || typeof sourceRoot !== 'string' ||
      goroot.includes('\0') || sourceRoot.includes('\0') ||
      !path.isAbsolute(goroot) || !path.isAbsolute(sourceRoot)) throw new Error('missing held overlay roots');
  const relative='src/internal/runtime/syscall/windows/servicelasso_root_windows_amd64.go';
  return Object.freeze({target:path.join(goroot,relative),
    backing:path.join(sourceRoot,'toolchain-overlay/go1.26.0',relative)});
}
export function windowsOwnedOverlayMappings(goroot,sourceRoot,queryRegionsGo){
  if(typeof queryRegionsGo!=='string'||!path.isAbsolute(queryRegionsGo)||queryRegionsGo.includes('\0'))throw new Error('missing reviewed generated query regions');
  const root=windowsRootOverlayMapping(goroot,sourceRoot);
  const directory=path.dirname(root.target),backing=path.dirname(root.backing);
  return Object.freeze([root,...['servicelasso_call_windows_amd64.go','servicelasso_call_windows_amd64.s',
    'servicelasso_endpoint_windows_amd64.go','servicelasso_endpoint_windows_amd64.s','servicelasso_arguments_windows_amd64.go',
    'servicelasso_private_windows_amd64.go',
    'servicelasso_adopt_windows_amd64.go',
    'servicelasso_suspend_windows_amd64.go','servicelasso_suspend_windows_amd64.s',
    'servicelasso_creator_memory_windows_amd64.go',
    'servicelasso_creator_memory_windows_amd64.s',
    'servicelasso_creator_extent_windows_amd64.go',
    'servicelasso_creator_extent_windows_amd64.s',
    'servicelasso_creator_tls_windows_amd64.go',
    'servicelasso_creator_tls_windows_amd64.s',
    'servicelasso_creator_export_windows_amd64.go',
    'servicelasso_creator_export_windows_amd64.s',
    'servicelasso_creator_delay_windows_amd64.go',
    'servicelasso_creator_import_windows_amd64.go',
    'servicelasso_creator_import_windows_amd64.s',
    'servicelasso_creator_relocation_windows_amd64.go',
    'servicelasso_creator_relocation_windows_amd64.s',
    'servicelasso_creator_nativepe_windows_amd64.go',
    'servicelasso_creator_nativepe_windows_amd64.s',
    'servicelasso_creator_view_windows_amd64.go',
    'servicelasso_creator_pe_windows_amd64.go',
    'servicelasso_creator_read_windows_amd64.go',
    'servicelasso_creator_read_windows_amd64.s',
    'servicelasso_creator_metadata_windows_amd64.go',
    'servicelasso_creator_metadata_windows_amd64.s',
    'servicelasso_creator_file_windows_amd64.go',
    'servicelasso_creator_file_windows_amd64.s',
    'servicelasso_creator_name_windows_amd64.go',
    'servicelasso_creator_name_windows_amd64.s',
    'servicelasso_creator_modules_windows_amd64.go',
    'servicelasso_creator_modules_windows_amd64.s',
    'servicelasso_creator_image_windows_amd64.go',
    'servicelasso_creator_image_windows_amd64.s',
    'servicelasso_mapping_windows_amd64.go','servicelasso_mapping_windows_amd64.s',
    'servicelasso_reader_windows_amd64.go','servicelasso_reader_windows_amd64.s',
    'servicelasso_decode_binding_windows_amd64.go',
    'servicelasso_creator_pe_windows_amd64_test.go',
    'servicelasso_decode_binding_windows_amd64_test.go',
    'servicelasso_export_windows_amd64_test.go','servicelasso_return_windows_amd64_test.go',
    'servicelasso_arguments_windows_amd64_test.go','servicelasso_private_windows_amd64_test.go'].map(name=>
    Object.freeze({target:path.join(directory,name),backing:path.join(backing,name)})),
    Object.freeze({target:path.join(directory,'servicelasso_query_regions_windows_amd64.go'),backing:queryRegionsGo}),
    Object.freeze({target:path.join(goroot,'src/os/servicelasso_suspend_windows_amd64.go'),
      backing:path.join(sourceRoot,'toolchain-overlay/go1.26.0/src/os/servicelasso_suspend_windows_amd64.go')})]);
}
// argv arrays remain separate from shell text. Caller must retain each actual
// spawn/stdio/result original and apply exact-command admission before execution.
export function nativeOwnerCompilePlan(platform, compiler, generatedEntry, outputDirectory, windowsRootHeader = null, windowsQueryHeader = null) {
  if (!['linux-x64','windows-x64'].includes(platform)) throw new Error('unsupported native owner platform');
  for (const value of [compiler,generatedEntry,outputDirectory]) {
    if (typeof value !== 'string' || !value.length || value.includes('\0')) throw new Error('missing native build path');
  }
  const linux=platform==='linux-x64';
  if ((!linux && (typeof windowsRootHeader !== 'string' || !windowsRootHeader.length || windowsRootHeader.includes('\0'))) ||
      (!linux && (typeof windowsQueryHeader!=='string'||!windowsQueryHeader.length||windowsQueryHeader.includes('\0'))) ||
      (linux && (windowsRootHeader !== null||windowsQueryHeader!==null))) throw new Error('missing exact Windows root build input');
  const includes=['-Inative/owner',...(linux?['-Ithird_party/libsepol/src','-Ithird_party/libsepol/include']:[])];
  const definitions=linux?['-DSERVICE_LASSO_OWNED_SEPOL','-D_GNU_SOURCE']:[];
  const commands=[];
  function compile(source,index,library) {
    const output=outputDirectory+'/'+index+'.o';
    const redirect=[...(library?['-include','native/owner/sepol_source_redirect.h']:[]),
      ...(!linux?['-include',windowsRootHeader,'-include',windowsQueryHeader]:[])];
    commands.push(Object.freeze({source,output,library,command:compiler,
      argv:Object.freeze(['-std=c11',...includes,...definitions,...redirect,'-c',source,'-o',output])}));
  }
  const owners=linux?roster.linuxOwner:roster.windowsOwner;
  owners.forEach((source,index)=>compile(source,'owner-'+index,false));
  if(linux)roster.linuxSepol.forEach((source,index)=>compile(source,'sepol-'+index,true));
  compile(generatedEntry,'candidate-entry',false);
  return Object.freeze({platform,commands:Object.freeze(commands),
    objects:Object.freeze(commands.map(row=>row.output)),
    linkLibraries:Object.freeze(linux?['-pthread']:['-ladvapi32']),
    generatedEntry,windowsRootHeader,windowsQueryHeader,privateGoPackage:'./cmd/service-lasso-tui',
    publicMember:linux?'service-lasso-tui':'service-lasso-tui.exe',
    privateMember:linux?'.service-lasso-tui/service-lasso-tui-go':'.service-lasso-tui/service-lasso-tui-go.exe'});
}

// SAME emitted profile supplies both translations. The generated Go source is
// a held overlay input, never written into or mistaken for the reviewed tree.
// This plan neither emits inputs nor establishes their issuer/qualification.
export function linuxCandidateBuildPlan({go,node,compiler,sourceRoot,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,generatedGo,generatedEntry,overlay,payload,objectDirectory,ownerOutput}) {
  const inputs={go,node,compiler,sourceRoot,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,generatedGo,generatedEntry,overlay,payload,objectDirectory,ownerOutput};
  for(const value of Object.values(inputs)) {
    if(typeof value!=='string'||!path.isAbsolute(value)||value.includes('\0'))throw new Error('missing held absolute Linux candidate input');
  }
  const outputs=[overlay,payload,objectDirectory,ownerOutput].map(value=>path.resolve(value));
  const sources=[profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,generatedGo,generatedEntry].map(value=>path.resolve(value));
  if(new Set([...outputs,...sources]).size!==outputs.length+sources.length)throw new Error('Linux candidate source/output paths alias');
  if([...outputs,...sources.slice(3)].some(value=>[go,node,compiler,sourceRoot].map(item=>path.resolve(item)).includes(value)))throw new Error('Linux candidate output aliases build input');
  const owner=nativeOwnerCompilePlan('linux-x64',compiler,generatedEntry,objectDirectory);
  const options=['-mod=readonly','-tags=service_lasso_native_candidate','-overlay='+overlay];
  return Object.freeze({
    environment:Object.freeze({GOOS:'linux',GOARCH:'amd64',GOAMD64:'v1',CGO_ENABLED:'0',GOENV:'off',GOTOOLCHAIN:'local',GOWORK:'off',GOFLAGS:'',GOEXPERIMENT:'',GOFIPS140:'off',GOCACHEPROG:''}),
    emit:Object.freeze({command:node,argv:Object.freeze([path.join(sourceRoot,'scripts/emit-linux-candidate-profile.mjs'),profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,generatedGo,generatedEntry])}),
    overlayMappings:Object.freeze([Object.freeze({target:path.join(sourceRoot,'internal/lifetime/zz_native_candidate_generated_linux.go'),backing:generatedGo})]),
    sourceInputs:Object.freeze({profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,generatedGo,generatedEntry,relation:'same-complete-original-profile-and-source-geometry'}),
    test:Object.freeze({command:go,argv:Object.freeze(['test',...options,'./...'])}),
    payload:Object.freeze({command:go,argv:Object.freeze(['build',...options,'-buildvcs=true','-trimpath','-o',payload,'./cmd/service-lasso-tui'])}),
    owner,
    link:Object.freeze({command:compiler,argv:Object.freeze([...owner.objects,...owner.linkLibraries,'-o',ownerOutput]),output:ownerOutput}),
    order:Object.freeze(['held-authentic-original-inputs','emit-paired-candidate-source','write-reviewed-overlay','test-candidate-source','complete-held-payload','compile-existing-native-owner','link-existing-native-owner']),
    prerequisites:Object.freeze(['entire-source-go','new-complete-input-root','exact-command-admission','genuine-containing-reservation','qualified-native-provider-and-policy']),
  });
}

// Ordered construction data, not an execution/admission API. The caller retains
// actual subprocess, held input/output and source-qualified provider originals.
export function windowsPayloadFirstBuildPlan({ go, node, compiler, goroot, sourceRoot, overlay,
  queryProfile,queryRegionsGo,queryHeader,payload, rootHeader, generatedEntry, objectDirectory, ownerOutput }) {
  for (const value of [go,node,compiler,goroot,sourceRoot,overlay,queryProfile,queryRegionsGo,queryHeader,payload,rootHeader,generatedEntry,objectDirectory,ownerOutput]) {
    if (typeof value !== 'string' || !value.length || value.includes('\0') ||
        !path.isAbsolute(value)) throw new Error('missing held absolute payload-first build input');
  }
  const destinations=[overlay,queryRegionsGo,queryHeader,payload,rootHeader,generatedEntry,ownerOutput].map(value=>path.resolve(value).toLowerCase());
  if(new Set(destinations).size!==destinations.length)throw new Error('overlapping payload-first output paths');
  if(destinations.includes(path.resolve(queryProfile).toLowerCase()))throw new Error('query profile aliases build output');
  const environment=Object.freeze({GOROOT:goroot,GOOS:'windows',GOARCH:'amd64',GOAMD64:'v1',
    CGO_ENABLED:'0',GOENV:'off',GOTOOLCHAIN:'local',GOWORK:'off',GOFLAGS:'',
    GOEXPERIMENT:'',GOFIPS140:'off',GOCACHEPROG:''});
  const owner=nativeOwnerCompilePlan('windows-x64',compiler,generatedEntry,objectDirectory,rootHeader,queryHeader);
  return Object.freeze({
    environment,
    queryRegions:Object.freeze({command:node,argv:Object.freeze([
      path.join(sourceRoot,'scripts/emit-windows-query-regions.mjs'),queryProfile,queryRegionsGo,queryHeader]),
      prerequisites:Object.freeze(['reviewed-profile-inputs','genuine-containing-reservation'])}),
    overlayMappings:windowsOwnedOverlayMappings(goroot,sourceRoot,queryRegionsGo),
    payload:Object.freeze({command:go,argv:Object.freeze(['build','-mod=readonly','-buildvcs=true',
      '-trimpath','-overlay='+overlay,'-ldflags=-linkmode=internal','-o',payload,'./cmd/service-lasso-tui'])}),
    locator:Object.freeze({source:'scripts/windows-native-root-locator.mjs',payload,rootHeader,
      command:node,argv:Object.freeze([path.join(sourceRoot,'scripts/emit-windows-native-root-header.mjs'),payload,rootHeader]),
      sourceExtent:WINDOWS_NATIVE_ROOT_EXTENT,extentOrigin:'source-owned lower root layout',requiresCompletedHeldPayload:true}),
    owner,
    link:Object.freeze({command:compiler,argv:Object.freeze([...owner.objects,...owner.linkLibraries,'-o',ownerOutput]),output:ownerOutput}),
    order:Object.freeze(['emit-reviewed-query-regions','complete-held-payload','derive-and-bind-private-root-header','compile-existing-native-owner','link-existing-native-owner']),
  });
}
