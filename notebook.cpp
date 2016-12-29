#include "QWidget"
#include "notebook.h"
#include "notebookpage.h"

Notebook::Notebook(QWidget *parent)
    : QWidget(parent)
{

}

NotebookPage
Notebook::AddPage()
{
    return new NotebookPage(this);
}
