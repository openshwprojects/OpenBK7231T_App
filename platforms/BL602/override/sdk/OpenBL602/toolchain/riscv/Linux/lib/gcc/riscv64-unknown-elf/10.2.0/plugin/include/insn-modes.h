/* Generated automatically from machmode.def and config/riscv/riscv-modes.def
   by genmodes.  */

#ifndef GCC_INSN_MODES_H
#define GCC_INSN_MODES_H

enum machine_mode
{
  E_VOIDmode,              /* machmode.def:189 */
#define HAVE_VOIDmode
#ifdef USE_ENUM_MODES
#define VOIDmode E_VOIDmode
#else
#define VOIDmode ((void) 0, E_VOIDmode)
#endif
  E_BLKmode,               /* machmode.def:193 */
#define HAVE_BLKmode
#ifdef USE_ENUM_MODES
#define BLKmode E_BLKmode
#else
#define BLKmode ((void) 0, E_BLKmode)
#endif
  E_CCmode,                /* machmode.def:231 */
#define HAVE_CCmode
#ifdef USE_ENUM_MODES
#define CCmode E_CCmode
#else
#define CCmode ((void) 0, E_CCmode)
#endif
  E_BImode,                /* machmode.def:196 */
#define HAVE_BImode
#ifdef USE_ENUM_MODES
#define BImode E_BImode
#else
#define BImode (scalar_int_mode ((scalar_int_mode::from_int) E_BImode))
#endif
  E_QImode,                /* machmode.def:204 */
#define HAVE_QImode
#ifdef USE_ENUM_MODES
#define QImode E_QImode
#else
#define QImode (scalar_int_mode ((scalar_int_mode::from_int) E_QImode))
#endif
  E_HImode,                /* machmode.def:205 */
#define HAVE_HImode
#ifdef USE_ENUM_MODES
#define HImode E_HImode
#else
#define HImode (scalar_int_mode ((scalar_int_mode::from_int) E_HImode))
#endif
  E_SImode,                /* machmode.def:206 */
#define HAVE_SImode
#ifdef USE_ENUM_MODES
#define SImode E_SImode
#else
#define SImode (scalar_int_mode ((scalar_int_mode::from_int) E_SImode))
#endif
  E_DImode,                /* machmode.def:207 */
#define HAVE_DImode
#ifdef USE_ENUM_MODES
#define DImode E_DImode
#else
#define DImode (scalar_int_mode ((scalar_int_mode::from_int) E_DImode))
#endif
  E_TImode,                /* machmode.def:208 */
#define HAVE_TImode
#ifdef USE_ENUM_MODES
#define TImode E_TImode
#else
#define TImode (scalar_int_mode ((scalar_int_mode::from_int) E_TImode))
#endif
  E_QQmode,                /* machmode.def:234 */
#define HAVE_QQmode
#ifdef USE_ENUM_MODES
#define QQmode E_QQmode
#else
#define QQmode (scalar_mode ((scalar_mode::from_int) E_QQmode))
#endif
  E_HQmode,                /* machmode.def:235 */
#define HAVE_HQmode
#ifdef USE_ENUM_MODES
#define HQmode E_HQmode
#else
#define HQmode (scalar_mode ((scalar_mode::from_int) E_HQmode))
#endif
  E_SQmode,                /* machmode.def:236 */
#define HAVE_SQmode
#ifdef USE_ENUM_MODES
#define SQmode E_SQmode
#else
#define SQmode (scalar_mode ((scalar_mode::from_int) E_SQmode))
#endif
  E_DQmode,                /* machmode.def:237 */
#define HAVE_DQmode
#ifdef USE_ENUM_MODES
#define DQmode E_DQmode
#else
#define DQmode (scalar_mode ((scalar_mode::from_int) E_DQmode))
#endif
  E_TQmode,                /* machmode.def:238 */
#define HAVE_TQmode
#ifdef USE_ENUM_MODES
#define TQmode E_TQmode
#else
#define TQmode (scalar_mode ((scalar_mode::from_int) E_TQmode))
#endif
  E_UQQmode,               /* machmode.def:240 */
#define HAVE_UQQmode
#ifdef USE_ENUM_MODES
#define UQQmode E_UQQmode
#else
#define UQQmode (scalar_mode ((scalar_mode::from_int) E_UQQmode))
#endif
  E_UHQmode,               /* machmode.def:241 */
#define HAVE_UHQmode
#ifdef USE_ENUM_MODES
#define UHQmode E_UHQmode
#else
#define UHQmode (scalar_mode ((scalar_mode::from_int) E_UHQmode))
#endif
  E_USQmode,               /* machmode.def:242 */
#define HAVE_USQmode
#ifdef USE_ENUM_MODES
#define USQmode E_USQmode
#else
#define USQmode (scalar_mode ((scalar_mode::from_int) E_USQmode))
#endif
  E_UDQmode,               /* machmode.def:243 */
#define HAVE_UDQmode
#ifdef USE_ENUM_MODES
#define UDQmode E_UDQmode
#else
#define UDQmode (scalar_mode ((scalar_mode::from_int) E_UDQmode))
#endif
  E_UTQmode,               /* machmode.def:244 */
#define HAVE_UTQmode
#ifdef USE_ENUM_MODES
#define UTQmode E_UTQmode
#else
#define UTQmode (scalar_mode ((scalar_mode::from_int) E_UTQmode))
#endif
  E_HAmode,                /* machmode.def:246 */
#define HAVE_HAmode
#ifdef USE_ENUM_MODES
#define HAmode E_HAmode
#else
#define HAmode (scalar_mode ((scalar_mode::from_int) E_HAmode))
#endif
  E_SAmode,                /* machmode.def:247 */
#define HAVE_SAmode
#ifdef USE_ENUM_MODES
#define SAmode E_SAmode
#else
#define SAmode (scalar_mode ((scalar_mode::from_int) E_SAmode))
#endif
  E_DAmode,                /* machmode.def:248 */
#define HAVE_DAmode
#ifdef USE_ENUM_MODES
#define DAmode E_DAmode
#else
#define DAmode (scalar_mode ((scalar_mode::from_int) E_DAmode))
#endif
  E_TAmode,                /* machmode.def:249 */
#define HAVE_TAmode
#ifdef USE_ENUM_MODES
#define TAmode E_TAmode
#else
#define TAmode (scalar_mode ((scalar_mode::from_int) E_TAmode))
#endif
  E_UHAmode,               /* machmode.def:251 */
#define HAVE_UHAmode
#ifdef USE_ENUM_MODES
#define UHAmode E_UHAmode
#else
#define UHAmode (scalar_mode ((scalar_mode::from_int) E_UHAmode))
#endif
  E_USAmode,               /* machmode.def:252 */
#define HAVE_USAmode
#ifdef USE_ENUM_MODES
#define USAmode E_USAmode
#else
#define USAmode (scalar_mode ((scalar_mode::from_int) E_USAmode))
#endif
  E_UDAmode,               /* machmode.def:253 */
#define HAVE_UDAmode
#ifdef USE_ENUM_MODES
#define UDAmode E_UDAmode
#else
#define UDAmode (scalar_mode ((scalar_mode::from_int) E_UDAmode))
#endif
  E_UTAmode,               /* machmode.def:254 */
#define HAVE_UTAmode
#ifdef USE_ENUM_MODES
#define UTAmode E_UTAmode
#else
#define UTAmode (scalar_mode ((scalar_mode::from_int) E_UTAmode))
#endif
  E_HFmode,                /* config/riscv/riscv-modes.def:25 */
#define HAVE_HFmode
#ifdef USE_ENUM_MODES
#define HFmode E_HFmode
#else
#define HFmode (scalar_float_mode ((scalar_float_mode::from_int) E_HFmode))
#endif
  E_SFmode,                /* machmode.def:226 */
#define HAVE_SFmode
#ifdef USE_ENUM_MODES
#define SFmode E_SFmode
#else
#define SFmode (scalar_float_mode ((scalar_float_mode::from_int) E_SFmode))
#endif
  E_DFmode,                /* machmode.def:227 */
#define HAVE_DFmode
#ifdef USE_ENUM_MODES
#define DFmode E_DFmode
#else
#define DFmode (scalar_float_mode ((scalar_float_mode::from_int) E_DFmode))
#endif
  E_TFmode,                /* config/riscv/riscv-modes.def:22 */
#define HAVE_TFmode
#ifdef USE_ENUM_MODES
#define TFmode E_TFmode
#else
#define TFmode (scalar_float_mode ((scalar_float_mode::from_int) E_TFmode))
#endif
  E_SDmode,                /* machmode.def:267 */
#define HAVE_SDmode
#ifdef USE_ENUM_MODES
#define SDmode E_SDmode
#else
#define SDmode (scalar_float_mode ((scalar_float_mode::from_int) E_SDmode))
#endif
  E_DDmode,                /* machmode.def:268 */
#define HAVE_DDmode
#ifdef USE_ENUM_MODES
#define DDmode E_DDmode
#else
#define DDmode (scalar_float_mode ((scalar_float_mode::from_int) E_DDmode))
#endif
  E_TDmode,                /* machmode.def:269 */
#define HAVE_TDmode
#ifdef USE_ENUM_MODES
#define TDmode E_TDmode
#else
#define TDmode (scalar_float_mode ((scalar_float_mode::from_int) E_TDmode))
#endif
  E_CQImode,               /* machmode.def:262 */
#define HAVE_CQImode
#ifdef USE_ENUM_MODES
#define CQImode E_CQImode
#else
#define CQImode (complex_mode ((complex_mode::from_int) E_CQImode))
#endif
  E_CHImode,               /* machmode.def:262 */
#define HAVE_CHImode
#ifdef USE_ENUM_MODES
#define CHImode E_CHImode
#else
#define CHImode (complex_mode ((complex_mode::from_int) E_CHImode))
#endif
  E_CSImode,               /* machmode.def:262 */
#define HAVE_CSImode
#ifdef USE_ENUM_MODES
#define CSImode E_CSImode
#else
#define CSImode (complex_mode ((complex_mode::from_int) E_CSImode))
#endif
  E_CDImode,               /* machmode.def:262 */
#define HAVE_CDImode
#ifdef USE_ENUM_MODES
#define CDImode E_CDImode
#else
#define CDImode (complex_mode ((complex_mode::from_int) E_CDImode))
#endif
  E_CTImode,               /* machmode.def:262 */
#define HAVE_CTImode
#ifdef USE_ENUM_MODES
#define CTImode E_CTImode
#else
#define CTImode (complex_mode ((complex_mode::from_int) E_CTImode))
#endif
  E_HCmode,                /* machmode.def:264 */
#define HAVE_HCmode
#ifdef USE_ENUM_MODES
#define HCmode E_HCmode
#else
#define HCmode (complex_mode ((complex_mode::from_int) E_HCmode))
#endif
  E_SCmode,                /* machmode.def:264 */
#define HAVE_SCmode
#ifdef USE_ENUM_MODES
#define SCmode E_SCmode
#else
#define SCmode (complex_mode ((complex_mode::from_int) E_SCmode))
#endif
  E_DCmode,                /* machmode.def:264 */
#define HAVE_DCmode
#ifdef USE_ENUM_MODES
#define DCmode E_DCmode
#else
#define DCmode (complex_mode ((complex_mode::from_int) E_DCmode))
#endif
  E_TCmode,                /* machmode.def:264 */
#define HAVE_TCmode
#ifdef USE_ENUM_MODES
#define TCmode E_TCmode
#else
#define TCmode (complex_mode ((complex_mode::from_int) E_TCmode))
#endif
  E_VNx2BImode,            /* config/riscv/riscv-modes.def:113 */
#define HAVE_VNx2BImode
#ifdef USE_ENUM_MODES
#define VNx2BImode E_VNx2BImode
#else
#define VNx2BImode ((void) 0, E_VNx2BImode)
#endif
  E_VNx4BImode,            /* config/riscv/riscv-modes.def:114 */
#define HAVE_VNx4BImode
#ifdef USE_ENUM_MODES
#define VNx4BImode E_VNx4BImode
#else
#define VNx4BImode ((void) 0, E_VNx4BImode)
#endif
  E_VNx8BImode,            /* config/riscv/riscv-modes.def:115 */
#define HAVE_VNx8BImode
#ifdef USE_ENUM_MODES
#define VNx8BImode E_VNx8BImode
#else
#define VNx8BImode ((void) 0, E_VNx8BImode)
#endif
  E_VNx16BImode,           /* config/riscv/riscv-modes.def:116 */
#define HAVE_VNx16BImode
#ifdef USE_ENUM_MODES
#define VNx16BImode E_VNx16BImode
#else
#define VNx16BImode ((void) 0, E_VNx16BImode)
#endif
  E_VNx32BImode,           /* config/riscv/riscv-modes.def:117 */
#define HAVE_VNx32BImode
#ifdef USE_ENUM_MODES
#define VNx32BImode E_VNx32BImode
#else
#define VNx32BImode ((void) 0, E_VNx32BImode)
#endif
  E_VNx64BImode,           /* config/riscv/riscv-modes.def:118 */
#define HAVE_VNx64BImode
#ifdef USE_ENUM_MODES
#define VNx64BImode E_VNx64BImode
#else
#define VNx64BImode ((void) 0, E_VNx64BImode)
#endif
  E_VNx128BImode,          /* config/riscv/riscv-modes.def:119 */
#define HAVE_VNx128BImode
#ifdef USE_ENUM_MODES
#define VNx128BImode E_VNx128BImode
#else
#define VNx128BImode ((void) 0, E_VNx128BImode)
#endif
  E_VNx16QImode,           /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx16QImode
#ifdef USE_ENUM_MODES
#define VNx16QImode E_VNx16QImode
#else
#define VNx16QImode ((void) 0, E_VNx16QImode)
#endif
  E_VNx2x16QImode,         /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x16QImode
#ifdef USE_ENUM_MODES
#define VNx2x16QImode E_VNx2x16QImode
#else
#define VNx2x16QImode ((void) 0, E_VNx2x16QImode)
#endif
  E_VNx3x16QImode,         /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x16QImode
#ifdef USE_ENUM_MODES
#define VNx3x16QImode E_VNx3x16QImode
#else
#define VNx3x16QImode ((void) 0, E_VNx3x16QImode)
#endif
  E_VNx4x16QImode,         /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x16QImode
#ifdef USE_ENUM_MODES
#define VNx4x16QImode E_VNx4x16QImode
#else
#define VNx4x16QImode ((void) 0, E_VNx4x16QImode)
#endif
  E_VNx5x16QImode,         /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x16QImode
#ifdef USE_ENUM_MODES
#define VNx5x16QImode E_VNx5x16QImode
#else
#define VNx5x16QImode ((void) 0, E_VNx5x16QImode)
#endif
  E_VNx6x16QImode,         /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x16QImode
#ifdef USE_ENUM_MODES
#define VNx6x16QImode E_VNx6x16QImode
#else
#define VNx6x16QImode ((void) 0, E_VNx6x16QImode)
#endif
  E_VNx7x16QImode,         /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x16QImode
#ifdef USE_ENUM_MODES
#define VNx7x16QImode E_VNx7x16QImode
#else
#define VNx7x16QImode ((void) 0, E_VNx7x16QImode)
#endif
  E_VNx8x16QImode,         /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x16QImode
#ifdef USE_ENUM_MODES
#define VNx8x16QImode E_VNx8x16QImode
#else
#define VNx8x16QImode ((void) 0, E_VNx8x16QImode)
#endif
  E_VNx8HImode,            /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx8HImode
#ifdef USE_ENUM_MODES
#define VNx8HImode E_VNx8HImode
#else
#define VNx8HImode ((void) 0, E_VNx8HImode)
#endif
  E_VNx2x8HImode,          /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x8HImode
#ifdef USE_ENUM_MODES
#define VNx2x8HImode E_VNx2x8HImode
#else
#define VNx2x8HImode ((void) 0, E_VNx2x8HImode)
#endif
  E_VNx3x8HImode,          /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x8HImode
#ifdef USE_ENUM_MODES
#define VNx3x8HImode E_VNx3x8HImode
#else
#define VNx3x8HImode ((void) 0, E_VNx3x8HImode)
#endif
  E_VNx4x8HImode,          /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x8HImode
#ifdef USE_ENUM_MODES
#define VNx4x8HImode E_VNx4x8HImode
#else
#define VNx4x8HImode ((void) 0, E_VNx4x8HImode)
#endif
  E_VNx5x8HImode,          /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x8HImode
#ifdef USE_ENUM_MODES
#define VNx5x8HImode E_VNx5x8HImode
#else
#define VNx5x8HImode ((void) 0, E_VNx5x8HImode)
#endif
  E_VNx6x8HImode,          /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x8HImode
#ifdef USE_ENUM_MODES
#define VNx6x8HImode E_VNx6x8HImode
#else
#define VNx6x8HImode ((void) 0, E_VNx6x8HImode)
#endif
  E_VNx7x8HImode,          /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x8HImode
#ifdef USE_ENUM_MODES
#define VNx7x8HImode E_VNx7x8HImode
#else
#define VNx7x8HImode ((void) 0, E_VNx7x8HImode)
#endif
  E_VNx8x8HImode,          /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x8HImode
#ifdef USE_ENUM_MODES
#define VNx8x8HImode E_VNx8x8HImode
#else
#define VNx8x8HImode ((void) 0, E_VNx8x8HImode)
#endif
  E_VNx4SImode,            /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx4SImode
#ifdef USE_ENUM_MODES
#define VNx4SImode E_VNx4SImode
#else
#define VNx4SImode ((void) 0, E_VNx4SImode)
#endif
  E_VNx2x4SImode,          /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x4SImode
#ifdef USE_ENUM_MODES
#define VNx2x4SImode E_VNx2x4SImode
#else
#define VNx2x4SImode ((void) 0, E_VNx2x4SImode)
#endif
  E_VNx3x4SImode,          /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x4SImode
#ifdef USE_ENUM_MODES
#define VNx3x4SImode E_VNx3x4SImode
#else
#define VNx3x4SImode ((void) 0, E_VNx3x4SImode)
#endif
  E_VNx4x4SImode,          /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x4SImode
#ifdef USE_ENUM_MODES
#define VNx4x4SImode E_VNx4x4SImode
#else
#define VNx4x4SImode ((void) 0, E_VNx4x4SImode)
#endif
  E_VNx5x4SImode,          /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x4SImode
#ifdef USE_ENUM_MODES
#define VNx5x4SImode E_VNx5x4SImode
#else
#define VNx5x4SImode ((void) 0, E_VNx5x4SImode)
#endif
  E_VNx6x4SImode,          /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x4SImode
#ifdef USE_ENUM_MODES
#define VNx6x4SImode E_VNx6x4SImode
#else
#define VNx6x4SImode ((void) 0, E_VNx6x4SImode)
#endif
  E_VNx7x4SImode,          /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x4SImode
#ifdef USE_ENUM_MODES
#define VNx7x4SImode E_VNx7x4SImode
#else
#define VNx7x4SImode ((void) 0, E_VNx7x4SImode)
#endif
  E_VNx8x4SImode,          /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x4SImode
#ifdef USE_ENUM_MODES
#define VNx8x4SImode E_VNx8x4SImode
#else
#define VNx8x4SImode ((void) 0, E_VNx8x4SImode)
#endif
  E_VNx2DImode,            /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx2DImode
#ifdef USE_ENUM_MODES
#define VNx2DImode E_VNx2DImode
#else
#define VNx2DImode ((void) 0, E_VNx2DImode)
#endif
  E_VNx2x2DImode,          /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x2DImode
#ifdef USE_ENUM_MODES
#define VNx2x2DImode E_VNx2x2DImode
#else
#define VNx2x2DImode ((void) 0, E_VNx2x2DImode)
#endif
  E_VNx3x2DImode,          /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x2DImode
#ifdef USE_ENUM_MODES
#define VNx3x2DImode E_VNx3x2DImode
#else
#define VNx3x2DImode ((void) 0, E_VNx3x2DImode)
#endif
  E_VNx4x2DImode,          /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x2DImode
#ifdef USE_ENUM_MODES
#define VNx4x2DImode E_VNx4x2DImode
#else
#define VNx4x2DImode ((void) 0, E_VNx4x2DImode)
#endif
  E_VNx5x2DImode,          /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x2DImode
#ifdef USE_ENUM_MODES
#define VNx5x2DImode E_VNx5x2DImode
#else
#define VNx5x2DImode ((void) 0, E_VNx5x2DImode)
#endif
  E_VNx6x2DImode,          /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x2DImode
#ifdef USE_ENUM_MODES
#define VNx6x2DImode E_VNx6x2DImode
#else
#define VNx6x2DImode ((void) 0, E_VNx6x2DImode)
#endif
  E_VNx7x2DImode,          /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x2DImode
#ifdef USE_ENUM_MODES
#define VNx7x2DImode E_VNx7x2DImode
#else
#define VNx7x2DImode ((void) 0, E_VNx7x2DImode)
#endif
  E_VNx8x2DImode,          /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x2DImode
#ifdef USE_ENUM_MODES
#define VNx8x2DImode E_VNx8x2DImode
#else
#define VNx8x2DImode ((void) 0, E_VNx8x2DImode)
#endif
  E_VNx32QImode,           /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx32QImode
#ifdef USE_ENUM_MODES
#define VNx32QImode E_VNx32QImode
#else
#define VNx32QImode ((void) 0, E_VNx32QImode)
#endif
  E_VNx2x32QImode,         /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x32QImode
#ifdef USE_ENUM_MODES
#define VNx2x32QImode E_VNx2x32QImode
#else
#define VNx2x32QImode ((void) 0, E_VNx2x32QImode)
#endif
  E_VNx3x32QImode,         /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x32QImode
#ifdef USE_ENUM_MODES
#define VNx3x32QImode E_VNx3x32QImode
#else
#define VNx3x32QImode ((void) 0, E_VNx3x32QImode)
#endif
  E_VNx4x32QImode,         /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x32QImode
#ifdef USE_ENUM_MODES
#define VNx4x32QImode E_VNx4x32QImode
#else
#define VNx4x32QImode ((void) 0, E_VNx4x32QImode)
#endif
  E_VNx16HImode,           /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx16HImode
#ifdef USE_ENUM_MODES
#define VNx16HImode E_VNx16HImode
#else
#define VNx16HImode ((void) 0, E_VNx16HImode)
#endif
  E_VNx2x16HImode,         /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x16HImode
#ifdef USE_ENUM_MODES
#define VNx2x16HImode E_VNx2x16HImode
#else
#define VNx2x16HImode ((void) 0, E_VNx2x16HImode)
#endif
  E_VNx3x16HImode,         /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x16HImode
#ifdef USE_ENUM_MODES
#define VNx3x16HImode E_VNx3x16HImode
#else
#define VNx3x16HImode ((void) 0, E_VNx3x16HImode)
#endif
  E_VNx4x16HImode,         /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x16HImode
#ifdef USE_ENUM_MODES
#define VNx4x16HImode E_VNx4x16HImode
#else
#define VNx4x16HImode ((void) 0, E_VNx4x16HImode)
#endif
  E_VNx8SImode,            /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx8SImode
#ifdef USE_ENUM_MODES
#define VNx8SImode E_VNx8SImode
#else
#define VNx8SImode ((void) 0, E_VNx8SImode)
#endif
  E_VNx2x8SImode,          /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x8SImode
#ifdef USE_ENUM_MODES
#define VNx2x8SImode E_VNx2x8SImode
#else
#define VNx2x8SImode ((void) 0, E_VNx2x8SImode)
#endif
  E_VNx3x8SImode,          /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x8SImode
#ifdef USE_ENUM_MODES
#define VNx3x8SImode E_VNx3x8SImode
#else
#define VNx3x8SImode ((void) 0, E_VNx3x8SImode)
#endif
  E_VNx4x8SImode,          /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x8SImode
#ifdef USE_ENUM_MODES
#define VNx4x8SImode E_VNx4x8SImode
#else
#define VNx4x8SImode ((void) 0, E_VNx4x8SImode)
#endif
  E_VNx4DImode,            /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx4DImode
#ifdef USE_ENUM_MODES
#define VNx4DImode E_VNx4DImode
#else
#define VNx4DImode ((void) 0, E_VNx4DImode)
#endif
  E_VNx2x4DImode,          /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x4DImode
#ifdef USE_ENUM_MODES
#define VNx2x4DImode E_VNx2x4DImode
#else
#define VNx2x4DImode ((void) 0, E_VNx2x4DImode)
#endif
  E_VNx3x4DImode,          /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x4DImode
#ifdef USE_ENUM_MODES
#define VNx3x4DImode E_VNx3x4DImode
#else
#define VNx3x4DImode ((void) 0, E_VNx3x4DImode)
#endif
  E_VNx4x4DImode,          /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x4DImode
#ifdef USE_ENUM_MODES
#define VNx4x4DImode E_VNx4x4DImode
#else
#define VNx4x4DImode ((void) 0, E_VNx4x4DImode)
#endif
  E_VNx2TImode,            /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx2TImode
#ifdef USE_ENUM_MODES
#define VNx2TImode E_VNx2TImode
#else
#define VNx2TImode ((void) 0, E_VNx2TImode)
#endif
  E_VNx2x2TImode,          /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x2TImode
#ifdef USE_ENUM_MODES
#define VNx2x2TImode E_VNx2x2TImode
#else
#define VNx2x2TImode ((void) 0, E_VNx2x2TImode)
#endif
  E_VNx3x2TImode,          /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x2TImode
#ifdef USE_ENUM_MODES
#define VNx3x2TImode E_VNx3x2TImode
#else
#define VNx3x2TImode ((void) 0, E_VNx3x2TImode)
#endif
  E_VNx4x2TImode,          /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x2TImode
#ifdef USE_ENUM_MODES
#define VNx4x2TImode E_VNx4x2TImode
#else
#define VNx4x2TImode ((void) 0, E_VNx4x2TImode)
#endif
  E_VNx64QImode,           /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx64QImode
#ifdef USE_ENUM_MODES
#define VNx64QImode E_VNx64QImode
#else
#define VNx64QImode ((void) 0, E_VNx64QImode)
#endif
  E_VNx2x64QImode,         /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x64QImode
#ifdef USE_ENUM_MODES
#define VNx2x64QImode E_VNx2x64QImode
#else
#define VNx2x64QImode ((void) 0, E_VNx2x64QImode)
#endif
  E_VNx32HImode,           /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx32HImode
#ifdef USE_ENUM_MODES
#define VNx32HImode E_VNx32HImode
#else
#define VNx32HImode ((void) 0, E_VNx32HImode)
#endif
  E_VNx2x32HImode,         /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x32HImode
#ifdef USE_ENUM_MODES
#define VNx2x32HImode E_VNx2x32HImode
#else
#define VNx2x32HImode ((void) 0, E_VNx2x32HImode)
#endif
  E_VNx16SImode,           /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx16SImode
#ifdef USE_ENUM_MODES
#define VNx16SImode E_VNx16SImode
#else
#define VNx16SImode ((void) 0, E_VNx16SImode)
#endif
  E_VNx2x16SImode,         /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x16SImode
#ifdef USE_ENUM_MODES
#define VNx2x16SImode E_VNx2x16SImode
#else
#define VNx2x16SImode ((void) 0, E_VNx2x16SImode)
#endif
  E_VNx8DImode,            /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx8DImode
#ifdef USE_ENUM_MODES
#define VNx8DImode E_VNx8DImode
#else
#define VNx8DImode ((void) 0, E_VNx8DImode)
#endif
  E_VNx2x8DImode,          /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x8DImode
#ifdef USE_ENUM_MODES
#define VNx2x8DImode E_VNx2x8DImode
#else
#define VNx2x8DImode ((void) 0, E_VNx2x8DImode)
#endif
  E_VNx4TImode,            /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx4TImode
#ifdef USE_ENUM_MODES
#define VNx4TImode E_VNx4TImode
#else
#define VNx4TImode ((void) 0, E_VNx4TImode)
#endif
  E_VNx2x4TImode,          /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x4TImode
#ifdef USE_ENUM_MODES
#define VNx2x4TImode E_VNx2x4TImode
#else
#define VNx2x4TImode ((void) 0, E_VNx2x4TImode)
#endif
  E_VNx128QImode,          /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx128QImode
#ifdef USE_ENUM_MODES
#define VNx128QImode E_VNx128QImode
#else
#define VNx128QImode ((void) 0, E_VNx128QImode)
#endif
  E_VNx64HImode,           /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx64HImode
#ifdef USE_ENUM_MODES
#define VNx64HImode E_VNx64HImode
#else
#define VNx64HImode ((void) 0, E_VNx64HImode)
#endif
  E_VNx32SImode,           /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx32SImode
#ifdef USE_ENUM_MODES
#define VNx32SImode E_VNx32SImode
#else
#define VNx32SImode ((void) 0, E_VNx32SImode)
#endif
  E_VNx16DImode,           /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx16DImode
#ifdef USE_ENUM_MODES
#define VNx16DImode E_VNx16DImode
#else
#define VNx16DImode ((void) 0, E_VNx16DImode)
#endif
  E_VNx8TImode,            /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx8TImode
#ifdef USE_ENUM_MODES
#define VNx8TImode E_VNx8TImode
#else
#define VNx8TImode ((void) 0, E_VNx8TImode)
#endif
  E_VNx256QImode,          /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx256QImode
#ifdef USE_ENUM_MODES
#define VNx256QImode E_VNx256QImode
#else
#define VNx256QImode ((void) 0, E_VNx256QImode)
#endif
  E_VNx128HImode,          /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx128HImode
#ifdef USE_ENUM_MODES
#define VNx128HImode E_VNx128HImode
#else
#define VNx128HImode ((void) 0, E_VNx128HImode)
#endif
  E_VNx64SImode,           /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx64SImode
#ifdef USE_ENUM_MODES
#define VNx64SImode E_VNx64SImode
#else
#define VNx64SImode ((void) 0, E_VNx64SImode)
#endif
  E_VNx32DImode,           /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx32DImode
#ifdef USE_ENUM_MODES
#define VNx32DImode E_VNx32DImode
#else
#define VNx32DImode ((void) 0, E_VNx32DImode)
#endif
  E_VNx16TImode,           /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx16TImode
#ifdef USE_ENUM_MODES
#define VNx16TImode E_VNx16TImode
#else
#define VNx16TImode ((void) 0, E_VNx16TImode)
#endif
  E_VNx8HFmode,            /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx8HFmode
#ifdef USE_ENUM_MODES
#define VNx8HFmode E_VNx8HFmode
#else
#define VNx8HFmode ((void) 0, E_VNx8HFmode)
#endif
  E_VNx2x8HFmode,          /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x8HFmode
#ifdef USE_ENUM_MODES
#define VNx2x8HFmode E_VNx2x8HFmode
#else
#define VNx2x8HFmode ((void) 0, E_VNx2x8HFmode)
#endif
  E_VNx3x8HFmode,          /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x8HFmode
#ifdef USE_ENUM_MODES
#define VNx3x8HFmode E_VNx3x8HFmode
#else
#define VNx3x8HFmode ((void) 0, E_VNx3x8HFmode)
#endif
  E_VNx4x8HFmode,          /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x8HFmode
#ifdef USE_ENUM_MODES
#define VNx4x8HFmode E_VNx4x8HFmode
#else
#define VNx4x8HFmode ((void) 0, E_VNx4x8HFmode)
#endif
  E_VNx5x8HFmode,          /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x8HFmode
#ifdef USE_ENUM_MODES
#define VNx5x8HFmode E_VNx5x8HFmode
#else
#define VNx5x8HFmode ((void) 0, E_VNx5x8HFmode)
#endif
  E_VNx6x8HFmode,          /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x8HFmode
#ifdef USE_ENUM_MODES
#define VNx6x8HFmode E_VNx6x8HFmode
#else
#define VNx6x8HFmode ((void) 0, E_VNx6x8HFmode)
#endif
  E_VNx7x8HFmode,          /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x8HFmode
#ifdef USE_ENUM_MODES
#define VNx7x8HFmode E_VNx7x8HFmode
#else
#define VNx7x8HFmode ((void) 0, E_VNx7x8HFmode)
#endif
  E_VNx8x8HFmode,          /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x8HFmode
#ifdef USE_ENUM_MODES
#define VNx8x8HFmode E_VNx8x8HFmode
#else
#define VNx8x8HFmode ((void) 0, E_VNx8x8HFmode)
#endif
  E_VNx4SFmode,            /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx4SFmode
#ifdef USE_ENUM_MODES
#define VNx4SFmode E_VNx4SFmode
#else
#define VNx4SFmode ((void) 0, E_VNx4SFmode)
#endif
  E_VNx2x4SFmode,          /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x4SFmode
#ifdef USE_ENUM_MODES
#define VNx2x4SFmode E_VNx2x4SFmode
#else
#define VNx2x4SFmode ((void) 0, E_VNx2x4SFmode)
#endif
  E_VNx3x4SFmode,          /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x4SFmode
#ifdef USE_ENUM_MODES
#define VNx3x4SFmode E_VNx3x4SFmode
#else
#define VNx3x4SFmode ((void) 0, E_VNx3x4SFmode)
#endif
  E_VNx4x4SFmode,          /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x4SFmode
#ifdef USE_ENUM_MODES
#define VNx4x4SFmode E_VNx4x4SFmode
#else
#define VNx4x4SFmode ((void) 0, E_VNx4x4SFmode)
#endif
  E_VNx5x4SFmode,          /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x4SFmode
#ifdef USE_ENUM_MODES
#define VNx5x4SFmode E_VNx5x4SFmode
#else
#define VNx5x4SFmode ((void) 0, E_VNx5x4SFmode)
#endif
  E_VNx6x4SFmode,          /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x4SFmode
#ifdef USE_ENUM_MODES
#define VNx6x4SFmode E_VNx6x4SFmode
#else
#define VNx6x4SFmode ((void) 0, E_VNx6x4SFmode)
#endif
  E_VNx7x4SFmode,          /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x4SFmode
#ifdef USE_ENUM_MODES
#define VNx7x4SFmode E_VNx7x4SFmode
#else
#define VNx7x4SFmode ((void) 0, E_VNx7x4SFmode)
#endif
  E_VNx8x4SFmode,          /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x4SFmode
#ifdef USE_ENUM_MODES
#define VNx8x4SFmode E_VNx8x4SFmode
#else
#define VNx8x4SFmode ((void) 0, E_VNx8x4SFmode)
#endif
  E_VNx2DFmode,            /* config/riscv/riscv-modes.def:56 */
#define HAVE_VNx2DFmode
#ifdef USE_ENUM_MODES
#define VNx2DFmode E_VNx2DFmode
#else
#define VNx2DFmode ((void) 0, E_VNx2DFmode)
#endif
  E_VNx2x2DFmode,          /* config/riscv/riscv-modes.def:83 */
#define HAVE_VNx2x2DFmode
#ifdef USE_ENUM_MODES
#define VNx2x2DFmode E_VNx2x2DFmode
#else
#define VNx2x2DFmode ((void) 0, E_VNx2x2DFmode)
#endif
  E_VNx3x2DFmode,          /* config/riscv/riscv-modes.def:84 */
#define HAVE_VNx3x2DFmode
#ifdef USE_ENUM_MODES
#define VNx3x2DFmode E_VNx3x2DFmode
#else
#define VNx3x2DFmode ((void) 0, E_VNx3x2DFmode)
#endif
  E_VNx4x2DFmode,          /* config/riscv/riscv-modes.def:85 */
#define HAVE_VNx4x2DFmode
#ifdef USE_ENUM_MODES
#define VNx4x2DFmode E_VNx4x2DFmode
#else
#define VNx4x2DFmode ((void) 0, E_VNx4x2DFmode)
#endif
  E_VNx5x2DFmode,          /* config/riscv/riscv-modes.def:86 */
#define HAVE_VNx5x2DFmode
#ifdef USE_ENUM_MODES
#define VNx5x2DFmode E_VNx5x2DFmode
#else
#define VNx5x2DFmode ((void) 0, E_VNx5x2DFmode)
#endif
  E_VNx6x2DFmode,          /* config/riscv/riscv-modes.def:87 */
#define HAVE_VNx6x2DFmode
#ifdef USE_ENUM_MODES
#define VNx6x2DFmode E_VNx6x2DFmode
#else
#define VNx6x2DFmode ((void) 0, E_VNx6x2DFmode)
#endif
  E_VNx7x2DFmode,          /* config/riscv/riscv-modes.def:88 */
#define HAVE_VNx7x2DFmode
#ifdef USE_ENUM_MODES
#define VNx7x2DFmode E_VNx7x2DFmode
#else
#define VNx7x2DFmode ((void) 0, E_VNx7x2DFmode)
#endif
  E_VNx8x2DFmode,          /* config/riscv/riscv-modes.def:89 */
#define HAVE_VNx8x2DFmode
#ifdef USE_ENUM_MODES
#define VNx8x2DFmode E_VNx8x2DFmode
#else
#define VNx8x2DFmode ((void) 0, E_VNx8x2DFmode)
#endif
  E_VNx16HFmode,           /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx16HFmode
#ifdef USE_ENUM_MODES
#define VNx16HFmode E_VNx16HFmode
#else
#define VNx16HFmode ((void) 0, E_VNx16HFmode)
#endif
  E_VNx2x16HFmode,         /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x16HFmode
#ifdef USE_ENUM_MODES
#define VNx2x16HFmode E_VNx2x16HFmode
#else
#define VNx2x16HFmode ((void) 0, E_VNx2x16HFmode)
#endif
  E_VNx3x16HFmode,         /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x16HFmode
#ifdef USE_ENUM_MODES
#define VNx3x16HFmode E_VNx3x16HFmode
#else
#define VNx3x16HFmode ((void) 0, E_VNx3x16HFmode)
#endif
  E_VNx4x16HFmode,         /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x16HFmode
#ifdef USE_ENUM_MODES
#define VNx4x16HFmode E_VNx4x16HFmode
#else
#define VNx4x16HFmode ((void) 0, E_VNx4x16HFmode)
#endif
  E_VNx8SFmode,            /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx8SFmode
#ifdef USE_ENUM_MODES
#define VNx8SFmode E_VNx8SFmode
#else
#define VNx8SFmode ((void) 0, E_VNx8SFmode)
#endif
  E_VNx2x8SFmode,          /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x8SFmode
#ifdef USE_ENUM_MODES
#define VNx2x8SFmode E_VNx2x8SFmode
#else
#define VNx2x8SFmode ((void) 0, E_VNx2x8SFmode)
#endif
  E_VNx3x8SFmode,          /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x8SFmode
#ifdef USE_ENUM_MODES
#define VNx3x8SFmode E_VNx3x8SFmode
#else
#define VNx3x8SFmode ((void) 0, E_VNx3x8SFmode)
#endif
  E_VNx4x8SFmode,          /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x8SFmode
#ifdef USE_ENUM_MODES
#define VNx4x8SFmode E_VNx4x8SFmode
#else
#define VNx4x8SFmode ((void) 0, E_VNx4x8SFmode)
#endif
  E_VNx4DFmode,            /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx4DFmode
#ifdef USE_ENUM_MODES
#define VNx4DFmode E_VNx4DFmode
#else
#define VNx4DFmode ((void) 0, E_VNx4DFmode)
#endif
  E_VNx2x4DFmode,          /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x4DFmode
#ifdef USE_ENUM_MODES
#define VNx2x4DFmode E_VNx2x4DFmode
#else
#define VNx2x4DFmode ((void) 0, E_VNx2x4DFmode)
#endif
  E_VNx3x4DFmode,          /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x4DFmode
#ifdef USE_ENUM_MODES
#define VNx3x4DFmode E_VNx3x4DFmode
#else
#define VNx3x4DFmode ((void) 0, E_VNx3x4DFmode)
#endif
  E_VNx4x4DFmode,          /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x4DFmode
#ifdef USE_ENUM_MODES
#define VNx4x4DFmode E_VNx4x4DFmode
#else
#define VNx4x4DFmode ((void) 0, E_VNx4x4DFmode)
#endif
  E_VNx2TFmode,            /* config/riscv/riscv-modes.def:57 */
#define HAVE_VNx2TFmode
#ifdef USE_ENUM_MODES
#define VNx2TFmode E_VNx2TFmode
#else
#define VNx2TFmode ((void) 0, E_VNx2TFmode)
#endif
  E_VNx2x2TFmode,          /* config/riscv/riscv-modes.def:91 */
#define HAVE_VNx2x2TFmode
#ifdef USE_ENUM_MODES
#define VNx2x2TFmode E_VNx2x2TFmode
#else
#define VNx2x2TFmode ((void) 0, E_VNx2x2TFmode)
#endif
  E_VNx3x2TFmode,          /* config/riscv/riscv-modes.def:92 */
#define HAVE_VNx3x2TFmode
#ifdef USE_ENUM_MODES
#define VNx3x2TFmode E_VNx3x2TFmode
#else
#define VNx3x2TFmode ((void) 0, E_VNx3x2TFmode)
#endif
  E_VNx4x2TFmode,          /* config/riscv/riscv-modes.def:93 */
#define HAVE_VNx4x2TFmode
#ifdef USE_ENUM_MODES
#define VNx4x2TFmode E_VNx4x2TFmode
#else
#define VNx4x2TFmode ((void) 0, E_VNx4x2TFmode)
#endif
  E_VNx32HFmode,           /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx32HFmode
#ifdef USE_ENUM_MODES
#define VNx32HFmode E_VNx32HFmode
#else
#define VNx32HFmode ((void) 0, E_VNx32HFmode)
#endif
  E_VNx2x32HFmode,         /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x32HFmode
#ifdef USE_ENUM_MODES
#define VNx2x32HFmode E_VNx2x32HFmode
#else
#define VNx2x32HFmode ((void) 0, E_VNx2x32HFmode)
#endif
  E_VNx16SFmode,           /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx16SFmode
#ifdef USE_ENUM_MODES
#define VNx16SFmode E_VNx16SFmode
#else
#define VNx16SFmode ((void) 0, E_VNx16SFmode)
#endif
  E_VNx2x16SFmode,         /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x16SFmode
#ifdef USE_ENUM_MODES
#define VNx2x16SFmode E_VNx2x16SFmode
#else
#define VNx2x16SFmode ((void) 0, E_VNx2x16SFmode)
#endif
  E_VNx8DFmode,            /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx8DFmode
#ifdef USE_ENUM_MODES
#define VNx8DFmode E_VNx8DFmode
#else
#define VNx8DFmode ((void) 0, E_VNx8DFmode)
#endif
  E_VNx2x8DFmode,          /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x8DFmode
#ifdef USE_ENUM_MODES
#define VNx2x8DFmode E_VNx2x8DFmode
#else
#define VNx2x8DFmode ((void) 0, E_VNx2x8DFmode)
#endif
  E_VNx4TFmode,            /* config/riscv/riscv-modes.def:58 */
#define HAVE_VNx4TFmode
#ifdef USE_ENUM_MODES
#define VNx4TFmode E_VNx4TFmode
#else
#define VNx4TFmode ((void) 0, E_VNx4TFmode)
#endif
  E_VNx2x4TFmode,          /* config/riscv/riscv-modes.def:95 */
#define HAVE_VNx2x4TFmode
#ifdef USE_ENUM_MODES
#define VNx2x4TFmode E_VNx2x4TFmode
#else
#define VNx2x4TFmode ((void) 0, E_VNx2x4TFmode)
#endif
  E_VNx64HFmode,           /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx64HFmode
#ifdef USE_ENUM_MODES
#define VNx64HFmode E_VNx64HFmode
#else
#define VNx64HFmode ((void) 0, E_VNx64HFmode)
#endif
  E_VNx32SFmode,           /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx32SFmode
#ifdef USE_ENUM_MODES
#define VNx32SFmode E_VNx32SFmode
#else
#define VNx32SFmode ((void) 0, E_VNx32SFmode)
#endif
  E_VNx16DFmode,           /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx16DFmode
#ifdef USE_ENUM_MODES
#define VNx16DFmode E_VNx16DFmode
#else
#define VNx16DFmode ((void) 0, E_VNx16DFmode)
#endif
  E_VNx8TFmode,            /* config/riscv/riscv-modes.def:59 */
#define HAVE_VNx8TFmode
#ifdef USE_ENUM_MODES
#define VNx8TFmode E_VNx8TFmode
#else
#define VNx8TFmode ((void) 0, E_VNx8TFmode)
#endif
  E_VNx128HFmode,          /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx128HFmode
#ifdef USE_ENUM_MODES
#define VNx128HFmode E_VNx128HFmode
#else
#define VNx128HFmode ((void) 0, E_VNx128HFmode)
#endif
  E_VNx64SFmode,           /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx64SFmode
#ifdef USE_ENUM_MODES
#define VNx64SFmode E_VNx64SFmode
#else
#define VNx64SFmode ((void) 0, E_VNx64SFmode)
#endif
  E_VNx32DFmode,           /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx32DFmode
#ifdef USE_ENUM_MODES
#define VNx32DFmode E_VNx32DFmode
#else
#define VNx32DFmode ((void) 0, E_VNx32DFmode)
#endif
  E_VNx16TFmode,           /* config/riscv/riscv-modes.def:60 */
#define HAVE_VNx16TFmode
#ifdef USE_ENUM_MODES
#define VNx16TFmode E_VNx16TFmode
#else
#define VNx16TFmode ((void) 0, E_VNx16TFmode)
#endif
  MAX_MACHINE_MODE,

  MIN_MODE_RANDOM = E_VOIDmode,
  MAX_MODE_RANDOM = E_BLKmode,

  MIN_MODE_CC = E_CCmode,
  MAX_MODE_CC = E_CCmode,

  MIN_MODE_INT = E_QImode,
  MAX_MODE_INT = E_TImode,

  MIN_MODE_PARTIAL_INT = E_VOIDmode,
  MAX_MODE_PARTIAL_INT = E_VOIDmode,

  MIN_MODE_FRACT = E_QQmode,
  MAX_MODE_FRACT = E_TQmode,

  MIN_MODE_UFRACT = E_UQQmode,
  MAX_MODE_UFRACT = E_UTQmode,

  MIN_MODE_ACCUM = E_HAmode,
  MAX_MODE_ACCUM = E_TAmode,

  MIN_MODE_UACCUM = E_UHAmode,
  MAX_MODE_UACCUM = E_UTAmode,

  MIN_MODE_FLOAT = E_HFmode,
  MAX_MODE_FLOAT = E_TFmode,

  MIN_MODE_DECIMAL_FLOAT = E_SDmode,
  MAX_MODE_DECIMAL_FLOAT = E_TDmode,

  MIN_MODE_COMPLEX_INT = E_CQImode,
  MAX_MODE_COMPLEX_INT = E_CTImode,

  MIN_MODE_COMPLEX_FLOAT = E_HCmode,
  MAX_MODE_COMPLEX_FLOAT = E_TCmode,

  MIN_MODE_VECTOR_BOOL = E_VNx2BImode,
  MAX_MODE_VECTOR_BOOL = E_VNx128BImode,

  MIN_MODE_VECTOR_INT = E_VNx16QImode,
  MAX_MODE_VECTOR_INT = E_VNx16TImode,

  MIN_MODE_VECTOR_FRACT = E_VOIDmode,
  MAX_MODE_VECTOR_FRACT = E_VOIDmode,

  MIN_MODE_VECTOR_UFRACT = E_VOIDmode,
  MAX_MODE_VECTOR_UFRACT = E_VOIDmode,

  MIN_MODE_VECTOR_ACCUM = E_VOIDmode,
  MAX_MODE_VECTOR_ACCUM = E_VOIDmode,

  MIN_MODE_VECTOR_UACCUM = E_VOIDmode,
  MAX_MODE_VECTOR_UACCUM = E_VOIDmode,

  MIN_MODE_VECTOR_FLOAT = E_VNx8HFmode,
  MAX_MODE_VECTOR_FLOAT = E_VNx16TFmode,

  NUM_MACHINE_MODES = MAX_MACHINE_MODE
};

#define CONST_MODE_NUNITS
#define CONST_MODE_PRECISION
#define CONST_MODE_SIZE
#define CONST_MODE_UNIT_SIZE
#define CONST_MODE_BASE_ALIGN
#define CONST_MODE_IBIT const
#define CONST_MODE_FBIT const

#define BITS_PER_UNIT (8)
#define MAX_BITSIZE_MODE_ANY_INT (16*BITS_PER_UNIT)
#define MAX_BITSIZE_MODE_ANY_MODE 8192
#define NUM_INT_N_ENTS 1
#define NUM_POLY_INT_COEFFS 2

#endif /* insn-modes.h */
