// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <iostream>
#include <vector>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

std::vector<std::string> inputVars;

StatementMatcher getcMatcher = callExpr(
  callee(functionDecl(hasName("getc"))),
  hasAncestor(
    binaryOperator(hasOperatorName("=")).bind("binaryOp")
  )
).bind("getcCall");

StatementMatcher scanfMatcher =
  callExpr(callee(functionDecl(hasName("scanf")))).bind("scanfCall");


class ScanfExtractor : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const CallExpr* ScanfCall = Result.Nodes.getNodeAs<CallExpr>("scanfCall")) {
      llvm::outs() << "Found a scanf call with variables:";
      for (unsigned i = 1; i < ScanfCall->getNumArgs(); i++) {
        const Expr* ArgExpr = ScanfCall->getArg(i);
        if (const UnaryOperator* UnOp = dyn_cast<UnaryOperator>(ArgExpr)) {
          if (UnOp->getOpcode() == UO_AddrOf) {
            const Expr* AddrExpr = UnOp->getSubExpr();
            if (const DeclRefExpr* VarRef = dyn_cast<DeclRefExpr>(AddrExpr)) {
              const VarDecl* Var = dyn_cast<VarDecl>(VarRef->getDecl());
              if (Var) {
                llvm::outs() << " " << Var->getName();
                inputVars.push_back(Var->getNameAsString());
              }
            }
          }
        } else if (const DeclRefExpr* VarRef = dyn_cast<DeclRefExpr>(ArgExpr)) {
          const VarDecl* Var = dyn_cast<VarDecl>(VarRef->getDecl());
          if (Var) {
            llvm::outs() << " " << Var->getNameAsString();
            inputVars.push_back(Var->getNameAsString());
          }
        }
      }
      llvm::outs() << "\n";
    }
  }
};

class GetcExtractor : public MatchFinder::MatchCallback {
public :
    virtual void run(const MatchFinder::MatchResult &Result) {
    const auto *BinaryOp = Result.Nodes.getNodeAs<BinaryOperator>("binaryOp");
    if (BinaryOp) {
      const auto *Var = dyn_cast<DeclRefExpr>(BinaryOp->getLHS()->IgnoreImpCasts());
      if (Var) {
        const auto *VarDecl = Var->getDecl();
        llvm::outs() << "Found getc() call assigned to variable: " << VarDecl->getNameAsString() << "\n";
        inputVars.push_back(VarDecl->getNameAsString());
      }
    }
    }
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  GetcExtractor getcHandler;
  ScanfExtractor scanfHandler;
  MatchFinder Finder;

  Finder.addMatcher(getcMatcher, &getcHandler);
  Finder.addMatcher(scanfMatcher, &scanfHandler);

  Tool.run(newFrontendActionFactory(&Finder).get());

  llvm::outs() << "Tool ran, input var list : ";

  for(std::string var : inputVars)
  {
     llvm::outs()<< var <<" ";
  }
  llvm::outs() <<"\n";
}
