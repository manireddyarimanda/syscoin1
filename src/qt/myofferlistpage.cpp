/*
 * Syscoin Developers 2015
 */
#include "myofferlistpage.h"
#include "ui_myofferlistpage.h"
#include "offertablemodel.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "bitcoingui.h"
#include "editofferdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
MyOfferListPage::MyOfferListPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MyOfferListPage),
    model(0),
    optionsModel(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->newOffer->setIcon(QIcon());
    ui->copyOffer->setIcon(QIcon());
    ui->exportButton->setIcon(QIcon());
#endif

	ui->buttonBox->setVisible(false);

    ui->labelExplanation->setText(tr("These are your registered Syscoin Offers. Offer operations (create, update) take 1 confirmation to appear in this table."));
	
	
    // Context menu actions
    QAction *copyOfferAction = new QAction(ui->copyOffer->text(), this);
    QAction *copyOfferValueAction = new QAction(tr("&Copy Title"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyOfferAction);
    contextMenu->addAction(copyOfferValueAction);
    contextMenu->addAction(editAction);
    contextMenu->addSeparator();

    // Connect signals for context menu actions
    connect(copyOfferAction, SIGNAL(triggered()), this, SLOT(on_copyOffer_clicked()));
    connect(copyOfferValueAction, SIGNAL(triggered()), this, SLOT(onCopyOfferValueAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(onEditAction()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    // Pass through accept action from button box
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

MyOfferListPage::~MyOfferListPage()
{
    delete ui;
}
void MyOfferListPage::showEvent ( QShowEvent * event )
{
    if(!walletModel) return;
    /*if(walletModel->getEncryptionStatus() == WalletModel::Locked)
	{
        ui->labelExplanation->setText(tr("<font color='red'>WARNING: Your wallet is currently locked. For security purposes you'll need to enter your passphrase in order to interact with Syscoin Offers. Because your wallet is locked you must manually refresh this table after creating or updating an Offer. </font> <a href=\"http://lockedwallet.syscoin.org\">more info</a><br><br>These are your registered Syscoin Offers. Offer updates take 1 confirmation to appear in this table."));
		ui->labelExplanation->setTextFormat(Qt::RichText);
		ui->labelExplanation->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->labelExplanation->setOpenExternalLinks(true);
    }*/
}
void MyOfferListPage::setModel(WalletModel *walletModel, OfferTableModel *model)
{
    this->model = model;
	this->walletModel = walletModel;
    if(!model) return;
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

  
		// Receive filter
		proxyModel->setFilterRole(OfferTableModel::TypeRole);
		proxyModel->setFilterFixedString(OfferTableModel::Offer);
       
    
		ui->tableView->setModel(proxyModel);
		ui->tableView->sortByColumn(0, Qt::AscendingOrder);
        ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Title, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Category, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Price, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Currency, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Qty, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setResizeMode(OfferTableModel::Expired, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Name, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Title, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Category, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Price, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Currency, QHeaderView::ResizeToContents);
	ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Qty, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(OfferTableModel::Expired, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created offer
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewOffer(QModelIndex,int,int)));

    selectionChanged();
}

void MyOfferListPage::setOptionsModel(ClientModel* clientmodel, OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
	this->clientModel = clientmodel;
}

void MyOfferListPage::on_copyOffer_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Name);
}

void MyOfferListPage::onCopyOfferValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, OfferTableModel::Title);
}

void MyOfferListPage::onEditAction()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditOfferDialog dlg(EditOfferDialog::EditOffer);
    dlg.setModel(walletModel, model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void MyOfferListPage::on_refreshButton_clicked()
{
    if(!model)
        return;
    model->refreshOfferTable();
}
void MyOfferListPage::on_newOffer_clicked()
{
    if(!model)
        return;

    EditOfferDialog dlg(EditOfferDialog::NewOffer, this);
    dlg.setModel(walletModel,model);
    if(dlg.exec())
    {
        newOfferToSelect = dlg.getOffer();
    }
}
void MyOfferListPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->hasSelection())
    {
        ui->copyOffer->setEnabled(true);
    }
    else
    {
        ui->copyOffer->setEnabled(false);
    }
}

void MyOfferListPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which offer was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(OfferTableModel::Name);

    foreach (QModelIndex index, indexes)
    {
        QVariant offer = table->model()->data(index);
        returnValue = offer.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no offer entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void MyOfferListPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Offer Data"), QString(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Offer", OfferTableModel::Name, Qt::EditRole);
    writer.addColumn("Title", OfferTableModel::Title, Qt::EditRole);
	writer.addColumn("Category", OfferTableModel::Category, Qt::EditRole);
	writer.addColumn("Price", OfferTableModel::Price, Qt::EditRole);
	writer.addColumn("Currency", OfferTableModel::Currency, Qt::EditRole);
	writer.addColumn("Qty", OfferTableModel::Qty, Qt::EditRole);
	writer.addColumn("Expired", OfferTableModel::Expired, Qt::EditRole);
    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}



void MyOfferListPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void MyOfferListPage::selectNewOffer(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, OfferTableModel::Name, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newOfferToSelect))
    {
        // Select row of newly created offer, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newOfferToSelect.clear();
    }
}
bool MyOfferListPage::handleURI(const QString& strURI)
{
 
    return false;
}