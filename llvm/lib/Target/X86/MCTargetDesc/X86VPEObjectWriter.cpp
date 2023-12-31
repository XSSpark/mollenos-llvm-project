//===-- X86WinCOFFObjectWriter.cpp - X86 Win COFF Writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/X86FixupKinds.h"
#include "MCTargetDesc/X86MCTargetDesc.h"
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCVPEObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class X86VPEObjectWriter : public MCVPEObjectTargetWriter {
public:
  X86VPEObjectWriter(bool Is64Bit);
  ~X86VPEObjectWriter() override = default;

  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsCrossSection,
                        const MCAsmBackend &MAB) const override;
};

} // end anonymous namespace

X86VPEObjectWriter::X86VPEObjectWriter(bool Is64Bit)
    : MCVPEObjectTargetWriter(Is64Bit ? COFF::IMAGE_FILE_MACHINE_AMD64
                                      : COFF::IMAGE_FILE_MACHINE_I386) {}

unsigned X86VPEObjectWriter::getRelocType(MCContext &Ctx,
                                              const MCValue &Target,
                                              const MCFixup &Fixup,
                                              bool IsCrossSection,
                                              const MCAsmBackend &MAB) const {
  unsigned FixupKind = Fixup.getKind();
  if (IsCrossSection) {
    if (FixupKind != FK_Data_4 && FixupKind != llvm::X86::reloc_signed_4byte) {
      Ctx.reportError(Fixup.getLoc(), "Cannot represent this expression");
      return COFF::IMAGE_REL_AMD64_ADDR32;
    }
    FixupKind = FK_PCRel_4;
  }

  MCSymbolRefExpr::VariantKind Modifier = Target.isAbsolute() ?
    MCSymbolRefExpr::VK_None : Target.getSymA()->getKind();

  if (getMachine() == COFF::IMAGE_FILE_MACHINE_AMD64) {
    switch (FixupKind) {
    case FK_PCRel_4:
    case X86::reloc_riprel_4byte:
    case X86::reloc_riprel_4byte_movq_load:
    case X86::reloc_riprel_4byte_relax:
    case X86::reloc_riprel_4byte_relax_rex:
    case X86::reloc_branch_4byte_pcrel:
      return COFF::IMAGE_REL_AMD64_REL32;
    case FK_Data_4:
    case X86::reloc_signed_4byte:
    case X86::reloc_signed_4byte_relax:
      if (Modifier == MCSymbolRefExpr::VK_COFF_IMGREL32)
        return COFF::IMAGE_REL_AMD64_ADDR32NB;
      if (Modifier == MCSymbolRefExpr::VK_SECREL)
        return COFF::IMAGE_REL_AMD64_SECREL;
      return COFF::IMAGE_REL_AMD64_ADDR32;
    case FK_Data_8:
      return COFF::IMAGE_REL_AMD64_ADDR64;
    case FK_SecRel_2:
      return COFF::IMAGE_REL_AMD64_SECTION;
    case FK_SecRel_4:
      return COFF::IMAGE_REL_AMD64_SECREL;
    default:
      Ctx.reportError(Fixup.getLoc(), "unsupported relocation type");
      return COFF::IMAGE_REL_AMD64_ADDR32;
    }
  } else if (getMachine() == COFF::IMAGE_FILE_MACHINE_I386) {
    switch (FixupKind) {
    case FK_PCRel_4:
    case X86::reloc_riprel_4byte:
    case X86::reloc_riprel_4byte_movq_load:
      return COFF::IMAGE_REL_I386_REL32;
    case FK_Data_4:
    case X86::reloc_signed_4byte:
    case X86::reloc_signed_4byte_relax:
      if (Modifier == MCSymbolRefExpr::VK_COFF_IMGREL32)
        return COFF::IMAGE_REL_I386_DIR32NB;
      if (Modifier == MCSymbolRefExpr::VK_SECREL)
        return COFF::IMAGE_REL_AMD64_SECREL;
      return COFF::IMAGE_REL_I386_DIR32;
    case FK_SecRel_2:
      return COFF::IMAGE_REL_I386_SECTION;
    case FK_SecRel_4:
      return COFF::IMAGE_REL_I386_SECREL;
    default:
      Ctx.reportError(Fixup.getLoc(), "unsupported relocation type");
      return COFF::IMAGE_REL_I386_DIR32;
    }
  } else
    llvm_unreachable("Unsupported COFF machine type.");
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createX86VPEObjectWriter(bool Is64Bit) {
  return std::make_unique<X86VPEObjectWriter>(Is64Bit);
}
