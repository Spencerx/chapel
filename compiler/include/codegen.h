/*
 * Copyright 2020-2025 Hewlett Packard Enterprise Development LP
 * Copyright 2004-2019 Cray Inc.
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "intents.h"
#include "baseAST.h"
#include "LayeredValueTable.h"
#include "type.h"

#include <list>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

#ifdef HAVE_LLVM

// forward declare some LLVM things.
namespace llvm {
  namespace legacy {
    class FunctionPassManager;
  }
}

namespace clang {
  namespace CodeGen {
    class CGFunctionInfo;
  }
}

// and some chpl frontend things
namespace chpl {
  namespace libraries {
    class LibraryFile;
  }
}

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Target/TargetMachine.h"

struct ClangInfo;
#include "llvmGlobalToWide.h"

#endif

#include "files.h"
#include "genret.h"

/* This class contains information helpful in generating
 * code for nested loops. */
struct LoopData
{
#ifdef HAVE_LLVM
  LoopData(llvm::MDNode *accessGroup, bool markMemoryOps)
    : accessGroup(accessGroup), markMemoryOps(markMemoryOps)
  { }
  llvm::MDNode* accessGroup;
  bool markMemoryOps; // mark load/store with the access group
#endif
};

/* Holds names of files used by LLVM codegen. */
struct LLVMGenFilenames {
  std::string moduleFilename;
  std::string preOptFilename;
  std::string opt1Filename;
  std::string opt2Filename;
  std::string artifactFilename;
  std::string gpuObjectFilenamePrefix;
  std::string outFilenamePrefix;
  std::string fatbinFilename;
};

/* GenInfo is meant to be a global variable which stores
 * the code generator state - e.g. FILE* to print C to
 * or LLVM module in which to generate.
 */
struct GenInfo {

  /* Stores information about precompiled llvm Modules */
  struct PrecompiledModule {
#ifdef HAVE_LLVM
    const chpl::libraries::LibraryFile* lf = nullptr;
    std::unique_ptr<llvm::Module> mod;
    // the names of the globals needed from this module
    std::vector<UniqueString> neededGlobalNames;
#endif
  };

  // If we're generating C, this is the FILE* to print to
  // TODO: Rename cfile to just 'file' since it's also used when
  //       generating Fortran and Python interfaces.
  FILE* cfile;
  // When generating C, sometimes the code generator needs
  // to introduce a temporary variable. When it does,
  // it saves them here.
  std::vector<std::string> cLocalDecls;
  // When generating C code *internal to a function*, we
  // need to possibly insert some temporaries above the code
  // for the function, so we can't print the function body
  // until after we have generated the entire function.
  // So, we accumulate statements here and when we finish
  // code generating the function, we print out
  // all of cLocalDecls and all of the cStatements.
  std::vector<std::string> cStatements;

  // We're always generating code for some AST element
  // This is the astloc for the statement under consideration
  // Normally, it refers to a statement
  int lineno;
  const char* filename;

  std::map<const char*, FnSymbol*> functionCNameAstrToSymbol;

#ifdef HAVE_LLVM
  // stores parsed C stuff for extern blocks
  std::unique_ptr<LayeredValueTable> lvt;

  // Once we get to code generation....
  llvm::Module *module;
  llvm::IRBuilder<> *irBuilder;
  llvm::MDBuilder *mdBuilder;
  llvm::TargetMachine* targetMachine;

  LLVMGenFilenames llvmGenFilenames;

  std::vector<LoopData> loopStack;
  std::vector<std::pair<llvm::AllocaInst*, llvm::Type*> > currentStackVariables;
  const clang::CodeGen::CGFunctionInfo* currentFunctionABI;

  // tbaa information
  llvm::MDNode* tbaaRootNode;
  llvm::MDNode* tbaaUnionsNode;

  // Information for no-alias metadata generation
  llvm::MDNode* noAliasDomain;
  std::map<Symbol*, llvm::MDNode*> noAliasScopes;
  std::map<Symbol*, llvm::MDNode*> noAliasScopeLists;
  std::map<Symbol*, llvm::MDNode*> noAliasLists;

  // Information used to generate code with fLLVMWideOpt. Instead of
  // generating wide pointers with puts and gets, we generate
  // address space 100 (e.g.) pointers and use loads, stores, or memcpy,
  // which will be optimized by normal LLVM optimizations. Then, an
  // LLVM pass translates these address space 100 pointers and operations
  // on them back into wide pointers and puts/gets.
  GlobalToWideInfo globalToWideInfo;

  // Optimizations to apply immediately after code-generating a fn
  // (this one is only set for LLVM_USE_OLD_PASSES)
  llvm::legacy::FunctionPassManager* FPM_postgen = nullptr;

  // Managers to optimize immediately after code-generating a fn
  // (these ones are used ifndef LLVM_USE_OLD_PASSES)
  llvm::LoopAnalysisManager* LAM = nullptr;
  llvm::FunctionAnalysisManager* FAM = nullptr;
  llvm::CGSCCAnalysisManager* CGAM = nullptr;
  llvm::ModuleAnalysisManager* MAM = nullptr;
  llvm::FunctionPassManager* FunctionSimplificationPM = nullptr;
  llvm::PassInstrumentationCallbacks* PIC = nullptr;
  llvm::StandardInstrumentations* SI = nullptr;

  // pointer to clang support info
  ClangInfo* clangInfo = nullptr;

  // When using a separately compiled .dyno file,
  // keep track of the LLVM IR modules that have been used
  // for the separately compiled information.
  std::map<UniqueString, PrecompiledModule> precompiledMods;
#endif

  GenInfo();
};


extern GenInfo* gGenInfo;
extern int      gMaxVMT;
extern int      gStmtCount;
extern bool     gCodegenGPU;

// Map from filename to an integer that will represent an unique ID for each
// generated GET/PUT
extern std::map<std::string, int> commIDMap;

// Freshly initialize gGenInfo, expecting it does not already exist.
void initializeGenInfo(void);

#ifdef HAVE_LLVM
void setupClang(GenInfo* info, std::string rtmain);
#endif

// These typedefs exist just to avoid needing ifdefs in fn prototypes
#ifdef HAVE_LLVM
#include "clang/CodeGen/CGFunctionInfo.h"
typedef clang::FunctionDecl* ClangFunctionDeclPtr;
typedef llvm::FunctionType* LlvmFunctionTypePtr;
#else
typedef void* ClangFunctionDeclPtr;
typedef void* LlvmFunctionTypePtr;
#endif

bool isBuiltinExternCFunction(const char* cname);
bool needsCodegenWrtGPU(FnSymbol* fn);

const char* legalizeName(const char* name);

std::string numToString(int64_t num);
std::string int64_to_string(int64_t i);
std::string uint64_to_string(uint64_t i);
std::string real_to_string(double num);
std::string zlineToString(BaseAST* ast);
void zlineToFileIfNeeded(BaseAST* ast, FILE* outfile);
const char* idCommentTemp(BaseAST* ast);
void genComment(const char* comment, bool push=false);
void flushStatements(void);

GenRet codegenCallExpr(const char* fnName);
GenRet codegenCallExpr(const char* fnName, GenRet a1);
GenRet codegenCallExpr(const char* fnName, GenRet a1, GenRet a2);
GenRet codegenCallExprWithArgs(const char* fnName,
                               std::vector<GenRet> & args,
                               FnSymbol* fnSym = nullptr,
                               ClangFunctionDeclPtr FD = nullptr,
                               bool defaultToValues = true);
GenRet codegenGetLocaleID(void);
GenRet codegenUseGlobal(const char* global);
GenRet codegenProcedurePointerFetch(Expr* baseExpr);
GenRet codegenValueMaybeDeref(Expr* baseExpr);
void   codegenGlobalInt64(const char* cname, int64_t value, bool isHeader,
                          bool isConstant=true);

bool argRequiresCPtr(IntentTag intent, Type* t, bool isReceiver);
bool argRequiresCPtr(ArgSymbol* formal);
bool argRequiresCPtr(const FunctionType::Formal* formal);

Type* getNamedTypeDuringCodegen(const char* name);
void setupDefaultFilenames(void);
void gatherTypesForCodegen(void);
GenRet codegenTypeByName(const char* type_name);

void registerPrimitiveCodegens();

void linkInDynoFiles();

void closeCodegenFiles();

#endif //CODEGEN_H
