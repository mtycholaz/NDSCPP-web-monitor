import { Component, Inject, inject } from '@angular/core';
import { MatButton } from '@angular/material/button';
import {
    MAT_DIALOG_DATA,
    MatDialog,
    MatDialogActions,
    MatDialogContent,
    MatDialogRef,
    MatDialogTitle,
} from '@angular/material/dialog';

import { ReorderColumnsComponent } from '../components/reorder-columns/reorder-columns.component';
import { Column } from '../models';

@Component({
    template: `
        <h2 matDialogTitle>Reorder Columns</h2>
        <mat-dialog-content>
            <app-reorder-columns [(columns)]="columns"></app-reorder-columns>
        </mat-dialog-content>
        <mat-dialog-actions>
            <button mat-button (click)="onCancel()">Cancel</button>
            <button mat-raised-button (click)="onSave()">Save</button>
        </mat-dialog-actions>
    `,
    imports: [
        MatButton,
        MatDialogContent,
        MatDialogActions,
        MatDialogTitle,
        ReorderColumnsComponent,
    ],
    standalone: true,
})
export class ReorderColumnsDialogComponent {
    columns: Column[] = inject(MAT_DIALOG_DATA).columns as Column[];
    dialogRef = inject(MatDialogRef);

    onCancel() {
        this.dialogRef.close();
    }

    onSave() {
        this.dialogRef.close(this.columns);
    }
}
