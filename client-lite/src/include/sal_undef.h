#pragma once

// SAL is not supported with compilers that aren't msvc, this file defs them as nothing so we can compile with gcc.

#ifndef _WIN32

#undef _In_
#define _In_

#undef _Out_
#define _Out_

#undef _Inout_
#define _Inout_

#undef _Inout_opt_
#define _Inout_opt_

#undef _Inout_updates_
#define _Inout_updates_(c)

#undef _In_z_
#define _In_z_

#undef _Inout_z_
#define _Inout_z_

#undef _In_reads_bytes_
#define _In_reads_bytes_(s)

#undef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_(s)

#undef _Out_writes_
#define _Out_writes_(s)

#undef _Out_opt_
#define _Out_opt_

#undef _Outptr_
#define _Outptr_

#undef _Outptr_result_nullonfailure_
#define _Outptr_result_nullonfailure_

#undef _Out_writes_bytes_to_
#define _Out_writes_bytes_to_(s, c)

#undef _Outptr_result_maybenull_
#define _Outptr_result_maybenull_

#undef _Outptr_result_maybenull_z_
#define _Outptr_result_maybenull_z_

#undef _Out_z_cap_
#define _Out_z_cap_(s)

#undef _Outptr_result_buffer_
#define _Outptr_result_buffer_(s)

#undef _Out_writes_bytes_
#define _Out_writes_bytes_(s)

#undef _Out_writes_opt_
#define _Out_writes_opt_(s)

#undef _Out_writes_bytes_to_opt_
#define _Out_writes_bytes_to_opt_(s, c)

#undef _Out_writes_z_
#define _Out_writes_z_(c)

#undef _Out_writes_opt_z_
#define _Out_writes_opt_z_(c)

#undef _In_reads_
#define _In_reads_(s)

#undef _In_opt_
#define _In_opt_

#undef _In_opt_z_
#define _In_opt_z_

#undef _In_reads_opt_
#define _In_reads_opt_(s)

#undef _In_range_
#define _In_range_(low, hi)

#undef _In_count_
#define _In_count_(c)

#undef _COM_Outptr_
#define _COM_Outptr_

#undef _Printf_format_string_
#define _Printf_format_string_

#undef _Must_inspect_result_
#define _Must_inspect_result_

#undef _Deref_out_range_
#define _Deref_out_range_(low, hi)

#undef __fallthrough
#define __fallthrough

#undef _Requires_shared_lock_held_
#define _Requires_shared_lock_held_(l)

#undef _Requires_exclusive_lock_held_
#define _Requires_exclusive_lock_held_(l)

#undef _Requires_no_locks_held_
#define _Requires_no_locks_held_

#undef _Field_range_
#define _Field_range_(low, hi)

#undef _Always_
#define _Always_(s)

#undef _Post_z_
#define _Post_z_

#undef _Analysis_assume
#define _Analysis_assume_

#undef _Null_terminated_
#define _Null_terminated_

#endif // !_WIN32

#undef _Memberinitializer_
#define _Memberinitializer_

