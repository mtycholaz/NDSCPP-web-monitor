import { CdkDrag, CdkDragDrop, CdkDragHandle, CdkDropList, moveItemInArray } from '@angular/cdk/drag-drop';
import { ChangeDetectionStrategy, Component, input, output } from '@angular/core';
import { MatIcon } from '@angular/material/icon';
import { MatList, MatListItem, MatListItemIcon } from '@angular/material/list';

import { Column } from '../../models';

@Component({
    selector: 'app-reorder-columns',
    templateUrl: './reorder-columns.component.html',
    styleUrls: ['./reorder-columns.component.scss'],
    changeDetection: ChangeDetectionStrategy.OnPush,
    standalone: true,
    imports: [
        MatList,
        MatListItem,
        MatListItemIcon,
        MatIcon,
        CdkDrag,
        CdkDragHandle,
        CdkDropList
    ]
})
export class ReorderColumnsComponent {

    columns = input<Column[]>();
    columnsChange = output<Column[]>();

    drop(event: CdkDragDrop<Column[]>) {
        const columns = this.columns() as Column[];
        moveItemInArray(columns, event.previousIndex, event.currentIndex);
        this.columnsChange.emit(columns);
    }
}