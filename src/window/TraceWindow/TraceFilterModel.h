#ifndef TRACEFILTER_H
#define TRACEFILTER_H

#include <QSortFilterProxyModel>

class TraceFilterModel : public QSortFilterProxyModel
{
public:
    explicit TraceFilterModel(QObject *parent = nullptr);


public slots:
    void setFilterText(QString filtertext);
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    QString _filterText;
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
};

#endif // TRACEFILTER_H
