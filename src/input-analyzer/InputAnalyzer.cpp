#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/Comment.h"
#include "clang/AST/CommentVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <iostream>
#include <vector>
#include <map>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang::comments;
using namespace llvm;

std::vector<std::string> inputVars;

// Create a map of var names to represent flowing value relatioship
std::map<std::string, std::string> valFlow;

StatementMatcher getcMatcher = callExpr(
  callee(functionDecl(hasName("getc"))),
  hasAncestor(
    binaryOperator(hasOperatorName("=")).bind("binaryOp")
  )
).bind("getcCall");

StatementMatcher scanfMatcher =
  callExpr(callee(functionDecl(hasName("scanf")))).bind("scanfCall");

// Define a matcher for label statements
StatementMatcher labelMatcher = labelStmt().bind("label");

StatementMatcher assignMatcher = binaryOperator(
    hasOperatorName("="),
    hasLHS(declRefExpr().bind("lhs")),
    hasRHS(expr(hasDescendant(declRefExpr())).bind("rhs"))
).bind("assign");

// Define the callback function to handle matched statements
class AssignHandler : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    // Get the matched AST nodes
    const auto *LhsDeclRef = Result.Nodes.getNodeAs<DeclRefExpr>("lhs");
    const auto *RhsExpr = Result.Nodes.getNodeAs<Expr>("rhs");
    if (LhsDeclRef && RhsExpr) {
      llvm::errs() << "Found assignment statement with LHS: " << LhsDeclRef->getNameInfo().getAsString() << "\n";
      llvm::errs() << "RHS DeclRefExprs: \n";
      const auto *RhsSubExpr = RhsExpr->IgnoreImplicit()->IgnoreParens();
      if (const auto *RhsDeclRef = dyn_cast<DeclRefExpr>(RhsSubExpr)) {
        llvm::errs() << "\t" << RhsDeclRef->getNameInfo().getAsString() << "\n";
      } 
      else if (const auto *binaryRHS = dyn_cast<BinaryOperator>(RhsSubExpr))
      {
          const auto* lhsRHS = dyn_cast<Expr>(binaryRHS->getLHS());
          const auto* lhsRHSI = lhsRHS->IgnoreImplicit()->IgnoreParens();
          const auto* rhsRHS = dyn_cast<Expr>(binaryRHS->getRHS());
          const auto* rhsRHSI = rhsRHS->IgnoreImplicit()->IgnoreParens();

          const auto* l = dyn_cast<DeclRefExpr>(lhsRHSI);
          const auto* r = dyn_cast<DeclRefExpr>(rhsRHSI);

          if (l || r) {
            if (l)
              llvm::errs() << "RHS LHS: " << l->getNameInfo().getName().getAsString() << "\n";
            if (r)
              llvm::errs() << "RHS RHS: " << r->getNameInfo().getName().getAsString() << "\n";
          }        
      }
    }
  }
};

class LabelPrinter : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    // Get the matched statement
    const LabelStmt *Label = Result.Nodes.getNodeAs<LabelStmt>("label");
    if (Label) {
      StringRef LabelName = Label->getName();
        if (LabelName.contains("key_point")) {
          // Print the label name and line number
          llvm::outs() << LabelName
                      << " at line " << Result.Context->getSourceManager().getSpellingLineNumber(Label->getBeginLoc())
                      << "\n";
      }
    }
  }
};


class CommentFinder : public clang::ASTConsumer {
public:
  CommentFinder(clang::ASTContext  &ASTContext) {
  }

  void HandleTranslationUnit(clang::ASTContext &context) final {
    auto comments = context.Comments.getCommentsInFile(
      context.getSourceManager().getMainFileID());
    llvm::outs() << context.Comments.empty() << "\n";
    if(!context.Comments.empty())
    {
      for (auto it = comments->begin(); it != comments->end(); it++) {
        clang::RawComment* comment = it->second;
        std::string source = comment->getFormattedText(context.getSourceManager(),
            context.getDiagnostics());
        llvm::outs() << "source:" << source << "\n";
      }
    }
  }
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance& CI, clang::StringRef) final {
    return std::unique_ptr<clang::ASTConsumer>(
        new CommentFinder(CI.getASTContext()));
  }
};

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
  LabelPrinter labelHandler;
  AssignHandler assignHandler;

  Finder.addMatcher(getcMatcher, &getcHandler);
  Finder.addMatcher(scanfMatcher, &scanfHandler);
  Finder.addMatcher(labelMatcher, &labelHandler);
  Finder.addMatcher(assignMatcher, &assignHandler);

  Tool.run(newFrontendActionFactory(&Finder).get());

  llvm::outs() << "Tool ran, input var list : ";

  for(std::string var : inputVars)
  {
     llvm::outs()<< var <<" ";
  }
  llvm::outs() <<"\n";

  return 0;
}
